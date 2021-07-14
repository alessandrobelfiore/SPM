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
typedef int (* iFunctionCall)(int args, vector<int> arr);

int mod(int a, int b) {
  int r = a - (int) (a / b) * b;
  return r < 0 ? (r + b) : r;
}

class Cell {
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
};

class Table {
  private:
    Cell* current;
    Cell* future;
    int width;
    int height;

  public:
    Table() {}

    // constructor
    Table(int height, int width):
      height(height), width(width) {
      int size = height * width;
      int column, row;
      current = new Cell[size];
      future = new Cell[size];
      for (int i = 0; i < size; i++) {
        row = i / width;
        column = i % width;
        current[i] = Cell(i, rand() % 2, row, column);
        future[i] = Cell(i, 0, row, column);
      }
    }

    Cell* getCurrent() { return current; }

    void setFuture(int index, int value) {
      future[index].setValue(value);
    }

    int getCellValue(int i) {
      return current[i].getValue();
    }

    // print current table config
    void printCurrent() {
      for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
          int v = current[i * width + j].getValue();
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
          int v = future[i * width + j].getValue();
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
    }

    int getNW(int i) { return width * (mod(current[i].getRow() - 1, width))   + (mod(current[i].getColumn() - 1, height)); } 
    int getN(int i) { return width  * (mod(current[i].getRow() - 1, width))   + (mod(current[i].getColumn(), height)); } 
    int getNE(int i) { return width * (mod(current[i].getRow() - 1, width))   + (mod(current[i].getColumn() + 1, height)); }
    int getW(int i) { return width  * (mod(current[i].getRow(), width))       + (mod(current[i].getColumn() - 1, height)); }
    int getE(int i) { return width  * (mod(current[i].getRow(), width))       + (mod(current[i].getColumn() + 1, height)); }
    int getSW(int i) { return width * (mod(current[i].getRow() + 1, width))   + (mod(current[i].getColumn() - 1, height)); }
    int getS(int i) { return width  * (mod(current[i].getRow() + 1, width))   + (mod(current[i].getColumn(), height)); }
    int getSE(int i) { return width * (mod(current[i].getRow() + 1, width))   + (mod(current[i].getColumn() + 1, height)); }

/*     void getNeighbours(int i, int* arr) {
      arr[0] = current[getNW(i)].getValue();
      arr[1] = current[getN(i)].getValue();
      arr[2] = current[getNE(i)].getValue();
      arr[3] = current[getW(i)].getValue();
      arr[4] = current[getE(i)].getValue();
      arr[5] = current[getSW(i)].getValue();
      arr[6] = current[getS(i)].getValue();
      arr[7] = current[getSE(i)].getValue();
    } */

    vector<int> getNeighbours(int i) {
      vector<int> arr = 
                {current[getNW(i)].getValue(),  current[getN(i)].getValue(), current[getNE(i)].getValue(), 
                 current[getW(i)].getValue(),   current[getE(i)].getValue(),
                 current[getSW(i)].getValue(),  current[getS(i)].getValue(), current[getSE(i)].getValue() };
      return arr;
    }
};

void execute(int& n, int start, int stop, Table& table, iFunctionCall& rule, int& nSteps, 
  atomic<int>& threadsR, atomic<int>& threadsD, condition_variable& ns, mutex& m, condition_variable& check) {
  for (int j = 0; j < nSteps; j++) {
    for (int i = start; i <= stop; i++) {
      int val = table.getCellValue(i);
      int nVal = rule(val, table.getNeighbours(i));
/*       int tmp[8]; 
      table.getNeighbours(i, tmp);
      int nVal = rule(val, tmp); */
      table.setFuture(i, nVal);
    }
    //cout << "Step: " << j << " ended" << endl;
    unique_lock<mutex> lock(m);
    if (++threadsR == n) {
      //cout << "Notifying main thread!" << endl;
      check.notify_all();
    }
    ns.wait(lock);
    // FIXME SPURIOUS WAKEUP -> thread counter
    //ns.wait(lock, [tR = threadsR.load(), n](){ cout << "Thread id:" << this_thread::get_id() << " tried to wake up but " << tR << " ? " << n << endl; return tR == n; });
  }
  //cout << "Thread: " << this_thread::get_id() << " is done" << endl;
  threadsD++;
  if (threadsD.load() == n) { /* cout << "Waking up all!" << endl; */ check.notify_all(); }
  return;
}

class Game {
  private:
    // game table
    Table table;
    // rules
    iFunctionCall rule;
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

  public:
    Game() {
    }
    // copy constructor (must be explicitly declared if class has non-copyable member)
    Game(const Game& obj) 
    {
      table = obj.table;
      rule = obj.rule;
      nw = obj.nw;
      nSteps = obj.nSteps;
      size = obj.size;
    }
    Game& operator=(const Game&& obj) //move constructor (must be explicitly declared if class has non-copyable member)
    {
      table = obj.table;
      rule = obj.rule;
      nw = obj.nw;
      nSteps = obj.nSteps;
      size = obj.size;
      return *this;
    }

    Game(int height, int width, iFunctionCall rule, int nw, int nSteps):
      rule(rule), nw(nw), nSteps(nSteps) {
        table = Table(height, width);
        size = height * width;
    }
    void run() {
      auto start = Clock::now();
      
      if (nw == 1) {
        for (int j = 0; j < nSteps; j++) {
          for (int i = 0; i < size; i++) {
            int val = table.getCellValue(i);
/*             int tmp[8]; 
            table.getNeighbours(i, tmp);
            int nVal = rule(val, tmp); */
            int nVal = rule(val, table.getNeighbours(i));
            table.setFuture(i, nVal);
          }
          //table.printCurrent();
          table.swapCurrentFuture();
        }
        return;
      }

      vector<thread*> tids(nw);
      int offset = size / nw;
      int remaining  =  size % nw;
      int start = 0;
      int stop = offset - 1;
      //  TODO REFACTOR
      atomic<int> threadsReady(0);
      atomic<int> threadsDone(0);
      for(int i = 0; i < nw; i++) {
        tids[i] = new thread(execute, ref(nw), start, stop, ref(table), ref(rule), ref(nSteps), 
                            ref(threadsReady), ref(threadsDone), ref(nextStep), ref(m), ref(check));
        start += offset;
        if (i == (nw - 2)) stop += offset + remaining;
        else stop += offset;
      }

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

      auto end = Clock::now();
      std::cout << "Computed in: " 
            << chrono::duration_cast<chrono::milliseconds>(end - start).count()
            << " milliseconds" << std::endl;
      return;
    }
};

int gameRule(int value, vector<int> neighValues) {
  int sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += neighValues[i];
  }
  //neighValues.clear();
  if (sum < 2) { return 0; }
  if ((sum == 2 || sum == 3) && value == 1) { return 1; }
  if (value == 0 && sum == 3) { return 1; }
  if (sum > 3) { return 0; }
  else return 0;
}

int aRule(int value, vector<int> neighValues) {
  return 0;
}

int oRule(int value, vector<int> neighValues) {
  return 0;
}

int main(int argc, char* argv[]) {
  // args
  if (argc != 5 && argc != 6) {
    cout << "Received " << argc - 1 << " of the minimum 4 arguments" << endl;
    cout << "Usage is " << argv[0] << " height width num_workers num_steps [rule_id]" << endl;
    return(-1);
  }

  iFunctionCall rules[3] = {
    gameRule,
    aRule,
    oRule
  };

  // height
  auto height   = atoi(argv[1]);
  // width
  auto width    = atoi(argv[2]);
  // number of workers
  auto nWorkers = atoi(argv[3]);
  // number of steps
  auto nSteps   = atoi(argv[4]);
  // rule to apply (default is game of life)
  auto rule = gameRule;
  if (argc == 6) {
    rule = rules[atoi(argv[5])];
  }
  // automaton setup
  Game g = Game(height, width, rule, nWorkers, nSteps);
  // timing the run
  g.run();

  return 0;
}