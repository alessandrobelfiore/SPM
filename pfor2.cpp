#include <iostream>
#include <iomanip>
#include <vector>
#include <math.h>  
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

using namespace std;
using namespace ff;

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
    vector<Cell>* current = new vector<Cell>();
    vector<Cell>* future = new vector<Cell>();
    int width;
    int height;

  public:
    Table() {}

    // constructor
    Table(int height, int width):
      height(height), width(width) {
      int size = height * width;
      int column, row;
      for (int i = 0; i < size; i++) {
        row = i / width;
        column = i % width;
        // TODO INITIALIZE CELL VALUES 
        // TODO SEED
        current->push_back(Cell(i, rand() % 2, row, column));
        future->push_back(Cell(i, 0, row, column));
      }
      //future = current;
    }

    vector<Cell>* getCurrent() { return current; }

    void setFuture(int index, int value) {
      future->at(index).setValue(value);
    }

    int getCellValue(int i) {
      return current->at(i).getValue();
    }

    // print current table config
    void printCurrent() {
      for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
          int v = current->at(i * width + j).getValue();
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
          int v = future->at(i * width + j).getValue();
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

    int getNW(int i) { return width * (mod(current->at(i).getRow() - 1, width))   + (mod(current->at(i).getColumn() - 1, height)); } 
    int getN(int i) { return width  * (mod(current->at(i).getRow() - 1, width))   + (mod(current->at(i).getColumn(), height)); } 
    int getNE(int i) { return width * (mod(current->at(i).getRow() - 1, width))   + (mod(current->at(i).getColumn() + 1, height)); }
    int getW(int i) { return width  * (mod(current->at(i).getRow(), width))       + (mod(current->at(i).getColumn() - 1, height)); }
    int getE(int i) { return width  * (mod(current->at(i).getRow(), width))       + (mod(current->at(i).getColumn() + 1, height)); }
    int getSW(int i) { return width * (mod(current->at(i).getRow() + 1, width))   + (mod(current->at(i).getColumn() - 1, height)); }
    int getS(int i) { return width  * (mod(current->at(i).getRow() + 1, width))   + (mod(current->at(i).getColumn(), height)); }
    int getSE(int i) { return width * (mod(current->at(i).getRow() + 1, width))   + (mod(current->at(i).getColumn() + 1, height)); }

    vector<int> getNeighbours(int i) {
      vector<int> arr = 
                {current->at(getNW(i)).getValue(),  current->at(getN(i)).getValue(), current->at(getNE(i)).getValue(), 
                 current->at(getW(i)).getValue(),   current->at(getE(i)).getValue(),
                 current->at(getSW(i)).getValue(),  current->at(getS(i)).getValue(), current->at(getSE(i)).getValue() };
      return arr;
    }
};

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
      bool schedoff = false;
      ff::ParallelFor pf(nw);
      ff::Stencil2D sten();
      if (schedoff) pf.disableScheduler(); // disables the use of the scheduler

      vector<vector<int>> vectors;
      for (int i = 0; i < nw; i++) {
        vectors.push_back(vector<int>());
      }
      // TODO Fix for not divisible
      int chunk = size / nw;
      /* cout << "chunksize: " << chunk << endl; */

      cout << "steps: " << nSteps << endl;
      for (int i = 0; i < nSteps; i++) {
        /* cout << "ENTERED with index: " << i << endl; */
        pf.parallel_for(  0, size,
                          1,
                          0,
                          [&](const long i) {
                            int val = table.getCellValue(i);
                            int nVal = rule(val, table.getNeighbours(i));
                            vectors[ceil(i / chunk)].push_back(nVal);
                            /* cout << "element: " << i << "set in " << ceil(i / chunk) << endl; */
                          });
        /* cout << "PARFOR DONE" << endl; */
        for (int k = 0; k < nw; k++) {
          for(std::vector<int>::size_type j = 0; j != vectors[k].size(); j++) {
            table.setFuture((k * chunk) + j, vectors[k][j]);
          }
          vectors[k].clear();
        }
        table.printCurrent();
        table.swapCurrentFuture();
        /* cout << "currently index: " << i << endl; */
      }
      table.printCurrent();
      cout << "SET" << endl;
      return;
    }
};

int gameRule(int value, vector<int> neighValues) {
  int sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += neighValues[i];
  }
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
  iFunctionCall rule = gameRule;
  if (argc == 6) {
    rule = rules[atoi(argv[5])];
  }
  
  ff::ffTime(ff::START_TIME);

  Game g = Game(height, width, rule, nWorkers, nSteps);
  g.run();

  ff::ffTime(ff::STOP_TIME);
    std::cout << "Computed in: " << std::setprecision(8) << ff::ffTime(ff::GET_TIME) << " ms"  << std::endl;
  return 0;
}