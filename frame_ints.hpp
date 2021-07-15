#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <cstdlib>
#include <chrono>

using namespace std;

typedef std::chrono::high_resolution_clock Clock;

int mod(int a, int b) {
  int r = a - (int) (a / b) * b;
  return r < 0 ? (r + b) : r;
}

/* class Cell {
  private: 
    int index;
    int row;
    int column;
    int value;
    
  public:
    Cell() = default;

    Cell(int index, int value, int row, int column): 
      index(index), value(value), row(row), column(column) {}

    Cell(int index, int row, int column):
      index(index), row(row), column(column) {}

    // getters
    int getIndex() { return index; }
    int getValue() { return value; }
    int getRow() { return row; }
    int getColumn() { return column; }

    // setters
    void setValue(int v) { value = v; }
}; */

class Table {
  private:
  // TRY shared point
    int* current;
    int* future;
    int width;
    int height;

  public:
    Table() {}

    // constructor
    Table(int height, int width):
      height(height), width(width) {
      int size = height * width;
      int column, row;
      current = new int[size];
      future = new int[size];
      for (int i = 0; i < size; i++) {
        row = i / width;
        column = i % width;
        current[i] = rand() % 2;
        //Cell(i, rand() % 2, row, column);
        future[i] = 0;
        // Cell(i, 0, row, column);
      }
    }

    int* getCurrent() { return current; }

    void setFuture(int index, int value) {
      future[index] = value;
    }

    int getCellValue(int i) {
      return current[i];
    }

    // print current table config
    void printCurrent() {
      for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
          int v = current[i * width + j];
          if (v == 0) cout << "-";
          else cout << "x";
        }
        cout << endl;
      }
      cout << endl;
    }

    // print "future" table config
    void printFuture() {
      for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
          int v = future[i * width + j];
          if (v == 0) cout << "-";
          else cout << "x";
        }
        cout << endl;
      }
      cout << endl;
    }

    void swapCurrentFuture() {
      auto tmp = current;
      current = future;
      future = tmp;
      // TODO
      // std::swap(current, future);
    }

    vector<int> getNeighbours(int i) {
      long column, row;
      row = i / width;
      column = i % width;
      long x1 = width * (mod(row - 1, width));
      long x2 = width * (mod(row + 1, width));
      long x3 = width * row;
      long y1 = mod(column - 1, height);
      long y2 = mod(column + 1, height);
      vector<int> arr = 
                {
                current[x1 + y1],  
                current[x1 + column], 
                current[x1 + y2], 
                current[x3 + y1],   
                current[x3 + y2],
                current[x2 + y1],
                current[x2 + column],
                current[x2 + y2],
                };
      return arr;
    }
};

class Game {
  private:
    // game table
    Table table;
    // number of workers
    int nw;
    // number of steps
    int nSteps;
    // number of Cells
    int size;
    // wait here until nextStep is executable
    condition_variable nextStep;
    // wait here until a thread has completed a step
    condition_variable check;
    // utility mutex
    mutex m;
    mutex m1;
    // number of threads ready to get to the next step
    atomic<int> threadsReady;
    // number of threads completed
    atomic<int> threadsDone;

  public:
    Game() {
    }
    // copy constructor (must be explicitly declared if class has non-copyable member)
    Game(const Game& obj) 
    {
      table = obj.table;
      nw = obj.nw;
      nSteps = obj.nSteps;
      size = obj.size;
    }
    Game& operator=(const Game&& obj) //move constructor (must be explicitly declared if class has non-copyable member)
    {
      table = obj.table;
      nw = obj.nw;
      nSteps = obj.nSteps;
      size = obj.size;
      return *this;
    }

    Game(int height, int width, int nw, int nSteps):
      nw(nw), nSteps(nSteps) {
        table = Table(height, width);
        size = height * width;
        threadsReady = 0;
        threadsDone = 0;
    }

    void execute(int start, int stop) {
      for (int j = 0; j < nSteps; j++) {
        for (int i = start; i <= stop; i++) {
          int val = table.getCellValue(i);
          int nVal = rule(val, table.getNeighbours(i));
          table.setFuture(i, nVal);
        }
        //cout << "Step: " << j << " ended" << endl;
        unique_lock<mutex> lock(m);
        if (++threadsReady == nw) {
          //cout << "Notifying main thread!" << endl;
          check.notify_all();
        }
        nextStep.wait(lock);
        // FIXME SPURIOUS WAKEUP -> thread counter
        //ns.wait(lock, [tR = threadsR.load(), n](){ cout << "Thread id:" << this_thread::get_id() << " tried to wake up but " << tR << " ? " << n << endl; return tR == n; });
      }
      //cout << "Thread: " << this_thread::get_id() << " is done" << endl;
      threadsDone++;
      if (threadsDone.load() == nw) { /* cout << "Waking up all!" << endl; */ check.notify_all(); }
      return;
    }
    
    // Passa come copia, se vuoi referenza aggiungi &, oppure && per movable
    virtual int rule(int val, vector<int> arr) { return 0; };

    double run() {
      //auto startTime = Clock::now();
      
      if (nw == 1) {
        for (int j = 0; j < nSteps; j++) {
          for (int i = 0; i < size; i++) {
            int val = table.getCellValue(i);
            int nVal = rule(val, table.getNeighbours(i));
            table.setFuture(i, nVal);
          }
          //table.printCurrent();
          table.swapCurrentFuture();
        }
        return 0;
      }

      vector<thread*> tids(nw);
      int offset = size / nw;
      int remaining  =  size % nw;
      int start = 0;
      int stop = offset - 1;
      //  TODO REFACTOR
      for(int i = 0; i < nw; i++) {
        tids[i] = new thread(&Game::execute, this, start, stop);
        start += offset;
        if (i == (nw - 2)) stop += offset + remaining;
        else stop += offset;
      }

      auto startTime = Clock::now();

      // if computation is not over
      while (threadsDone.load() != nw) {
        // check if threads are all ready for the next step
        unique_lock<mutex> lock(m);
        /* cout << "Threads done: " << threadsDone.load() << endl; */
        //lock.lock();
        while (threadsReady.load() < nw) {
          //cout << threadsReady.load() << "=/=" << nw << endl;
          /* cout << "Going to sleep" << endl; */
          check.wait(lock);
          /* cout << "Woken up, tR: " << threadsReady.load() << "tD: " << threadsDone.load() << endl; */
          if (threadsDone.load() == nw) break;
        }
        if (threadsDone.load() == nw) break;
        //lock.unlock();
        table.swapCurrentFuture();
        //table.printCurrent();
        threadsReady.exchange(0);
        // send wake up signals
        nextStep.notify_all();
      }

      for(auto e : tids)
        e->join();

      auto endTime = Clock::now();

      /* table.printCurrent();
      table.printFuture(); */
      return chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
    }

};
