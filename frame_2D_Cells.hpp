#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <cstdlib>
#include <chrono>

typedef std::chrono::high_resolution_clock Clock;

using namespace std;
using pair_t = std::pair<int, int>;


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

/* redefining vectors of cells as rows */
using row = std::vector<Cell>;

class Table {
  private:
    // TRY shared point
    // 2D matrix
    vector<row>* current_rows = new vector<row>();
    vector<row>* future_rows = new vector<row>();
    long width;
    long height;
    long size;

  public:
    Table() {}

    // constructor
    Table(long height, long width):
      height(height), width(width) {
      size = height * width;
    }

    // populate rows
    void generate() {
      for (long i = 0; i < height; i++) {
        current_rows->push_back(vector<Cell>());
        future_rows->push_back(vector<Cell>());
      }

      for (long i = 0; i < size; i++) {
        long column, row;
        row = i / width;
        column = i % width;
        current_rows->at(row).push_back(Cell(i, rand() % 2, row, column));
        future_rows->at(row).push_back(Cell(i, 0, row, column));
      }
    }

    vector<row>* getCurrent() { return current_rows; }
    vector<row>* getFuture() { return future_rows; }

    void setFuture(long row, long column, int value) {
      future_rows->at(row)[column].setValue(value);
    }

    void setCurrent(long row, long column, int value) {
      current_rows->at(row)[column].setValue(value);
    }

    /* void setFuture(long i, int value) {
      long column, row;
      row = i / width;
      column = i % width;
      future_rows->at(row)[column].setValue(value);
    } */

    /* void setCurrent(long i, int value) {
      long column, row;
      row = i / width;
      column = i % width;
      current_rows->at(row)[column].setValue(value);
    } */

    int getCellValue(long row, long column) {
      return current_rows->at(row)[column].getValue();
    }

    /* int getCellValue(long i) {
      long column, row;
      row = i / width;
      column = i % width;
      return current_rows->at(row)[column].getValue();
    } */
    
    long getSize() { return size; }
    long getHeight() { return height; }
    long getWidth() { return width; }

    row getRow(long row) {
      return current_rows->at(row);
    }
    
    // print current table config
    void printCurrent() {
      for (long i = 0; i < height; i++) {
        for (long j = 0; j < width; j++) {
          int v = current_rows->at(i).at(j).getValue();
          if (v == 0) cout << "-";
          else cout << "x";
        }
        cout << endl;
      }
      cout << endl;
    }

    // print "future" table config
    void printFuture() {
      for (long i = 0; i < height; i++) {
        for (long j = 0; j < width; j++) {
          int v = future_rows->at(i).at(j).getValue();
          if (v == 0) cout << "-";
          else cout << "x";
        }
        cout << endl;
      }
      cout << endl;
    }

    void swapCurrentFuture() {
      auto tmp = current_rows;
      current_rows = future_rows;
      future_rows = tmp;
    }

    // optimized
    /* int getNW(int i) {  return width * (mod(current[i].getRow() - 1, width))   + (mod(current[i].getColumn() - 1, height)); } 
    int getN(int i) {   return width * (mod(current[i].getRow() - 1, width))   + (mod(current[i].getColumn(), height)); } 
    int getNE(int i) {  return width * (mod(current[i].getRow() - 1, width))   + (mod(current[i].getColumn() + 1, height)); }
    int getW(int i) {   return width * (mod(current[i].getRow(), width))       + (mod(current[i].getColumn() - 1, height)); }
    int getE(int i) {   return width * (mod(current[i].getRow(), width))       + (mod(current[i].getColumn() + 1, height)); }
    int getSW(int i) {  return width * (mod(current[i].getRow() + 1, width))   + (mod(current[i].getColumn() - 1, height)); }
    int getS(int i) {   return width * (mod(current[i].getRow() + 1, width))   + (mod(current[i].getColumn(), height)); }
    int getSE(int i) {  return width * (mod(current[i].getRow() + 1, width))   + (mod(current[i].getColumn() + 1, height)); } */

    pair_t getNW(long row, long column) { return {mod(row - 1, height), mod(column - 1, width)}; } 
    pair_t getN(long row, long column) {  return {mod(row - 1, height), column}; } 
    pair_t getNE(long row, long column) { return {mod(row - 1, height), mod(column + 1, width)}; }
    pair_t getW(long row, long column) {  return {row, mod(column - 1, width)}; }
    pair_t getE(long row, long column) {  return {row, mod(column + 1, width)}; }
    pair_t getSW(long row, long column) { return {mod(row + 1, height), mod(column - 1, width)}; }
    pair_t getS(long row, long column) {  return {mod(row + 1, height), column}; }
    pair_t getSE(long row, long column) { return {mod(row + 1, height), mod(column + 1, width)}; }

    vector<int> getNeighbours(long row, long column) {
      vector<int> arr = 
                {current_rows->at(getNW(row, column).first).at(getNW(row, column).second).getValue(),  current_rows->at(getN(row, column).first).at(getN(row, column).second).getValue(), current_rows->at(getNE(row, column).first).at(getNE(row, column).second).getValue(), 
                 current_rows->at(getW(row, column).first).at(getW(row, column).second).getValue(),   current_rows->at(getE(row, column).first).at(getE(row, column).second).getValue(),
                 current_rows->at(getSW(row, column).first).at(getSW(row, column).second).getValue(),  current_rows->at(getS(row, column).first).at(getS(row, column).second).getValue(), current_rows->at(getSE(row, column).first).at(getSE(row, column).second).getValue() };
      return arr;
    }

    vector<int> getNeighbours2(long row, long column) {
      vector<int> arr = 
        {
        (*current_rows)[(mod(row - 1, height))][mod(column - 1, width)].getValue(), (*current_rows)[(mod(row - 1, height))][column].getValue(), (*current_rows)[(mod(row - 1, height))][mod(column + 1, width)].getValue(),
        (*current_rows)[row][mod(column - 1, width)].getValue(), (*current_rows)[row][mod(column + 1, width)].getValue(),
        (*current_rows)[(mod(row + 1, height))][mod(column - 1, width)].getValue(), (*current_rows)[(mod(row + 1, height))][column].getValue(), (*current_rows)[(mod(row + 1, height))][mod(column + 1, width)].getValue(),
        };
      return arr;
    }

    vector<int> getNeighbours3(long row, long column) {
      vector<int> arr = 
        {
        (*current_rows)[(row - 1 + height) % height][(column - 1 + width) % width].getValue(),
        (*current_rows)[(row - 1 + height) % height][column].getValue(), 
        (*current_rows)[(row - 1 + height) % height][(column + 1) % width].getValue(),
        (*current_rows)[row][(column - 1 + width) % width].getValue(), 
        (*current_rows)[row][(column + 1) % width].getValue(),
        (*current_rows)[(row + 1 ) % height][(column - 1 + width) % width].getValue(), 
        (*current_rows)[(row + 1 ) % height][column].getValue(), 
        (*current_rows)[(row + 1 ) % height][(column + 1) % width].getValue(),
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
    int height;
    int width;
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
      height(height), width(width), nw(nw), nSteps(nSteps) {
        table = Table(height, width);
        table.generate();
        size = height * width;
        threadsReady = 0;
        threadsDone = 0;
    }

    void execute(int rows_start, int rows_stop, int width) {
      for (int j = 0; j < nSteps; j++) {
        // vector<int> neighbours(8);
        for (int i = rows_start; i < rows_stop; i++) {
          for (int j = 0; j < width; j++) {
            int val = table.getCellValue(i, j);
            int nVal = rule(val, table.getNeighbours2(i, j));
            table.setFuture(i, j, nVal);
          }
        }
        unique_lock<mutex> lock(m);
        if (++threadsReady == nw) {
          check.notify_all();
        }
        nextStep.wait(lock);
      }
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
          // vector<int> neighbours(8);
          for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
              int val = table.getCellValue(i, j);
              int nVal = rule(val, table.getNeighbours2(i, j));
              table.setFuture(i, j, nVal);
            }
          }
          //table.printCurrent();
          table.swapCurrentFuture();
        }
        return 0;
      }

      vector<thread*> tids(nw);
      long s_height = height / nw;   // actual height of the current subtable (including the last one)
      long remaining  = height % nw;
      long start = 0;
      long stop = s_height;
      //  TODO REFACTOR
      for(int i = 0; i < nw; i++) {
        tids[i] = new thread(&Game::execute, this, start, stop, width);
        start += s_height;
        if (i == (nw - 2)) stop += s_height + remaining;
        else stop += s_height;
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
      return chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
    }

};
