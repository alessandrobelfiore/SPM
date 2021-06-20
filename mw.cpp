#include <iostream>
#include <iomanip>
#include <vector>
#include <ff/ff.hpp>
#include <ff/farm.hpp>

using namespace std;

typedef int (* iFunctionCall)(int args, vector<int> arr);

using pair_t = std::pair<int,int>;

int mod(int a, int b) {
  int r = a - (int) (a / b) * b;
  return r < 0 ? (r + b) : r;
}

/* Class representing a cell
  index:  index in the 1D matrix 
  row:    row index in the 2D matrix
  column: column index in the 2D matrix
  value:  value contained in the cell  */
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
    long width;
    long height;

  public:
    Table() {}

    // constructor
    Table(long height, long width):
      height(height), width(width) {
      long size = height * width;
      long column, row;
      for (long i = 0; i < size; i++) {
        row = i / width;
        column = i % width;
        current->push_back(Cell(i, rand() % 2, row, column));
        future->push_back(Cell(i, 0, row, column));
      }
    }

    vector<Cell>* getCurrent() { return current; }

    void setFuture(long index, int value) {
      future->at(index).setValue(value);
    }

    int getCellValue(long i) {
      return current->at(i).getValue();
    }

    // print current table config
    void printCurrent() {
      for (long i = 0; i < height; i++) {
        for (long j = 0; j < width; j++) {
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
      for (long i = 0; i < height; i++) {
        for (long j = 0; j < width; j++) {
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

    int getNW(long i) { return width * (mod(current->at(i).getRow() - 1, width))   + (mod(current->at(i).getColumn() - 1, height)); } 
    int getN(long i) { return width  * (mod(current->at(i).getRow() - 1, width))   + (mod(current->at(i).getColumn(), height)); } 
    int getNE(long i) { return width * (mod(current->at(i).getRow() - 1, width))   + (mod(current->at(i).getColumn() + 1, height)); }
    int getW(long i) { return width  * (mod(current->at(i).getRow(), width))       + (mod(current->at(i).getColumn() - 1, height)); }
    int getE(long i) { return width  * (mod(current->at(i).getRow(), width))       + (mod(current->at(i).getColumn() + 1, height)); }
    int getSW(long i) { return width * (mod(current->at(i).getRow() + 1, width))   + (mod(current->at(i).getColumn() - 1, height)); }
    int getS(long i) { return width  * (mod(current->at(i).getRow() + 1, width))   + (mod(current->at(i).getColumn(), height)); }
    int getSE(long i) { return width * (mod(current->at(i).getRow() + 1, width))   + (mod(current->at(i).getColumn() + 1, height)); }

    vector<int> getNeighbours(long i) {
      vector<int> arr = 
                {current->at(getNW(i)).getValue(),  current->at(getN(i)).getValue(), current->at(getNE(i)).getValue(), 
                 current->at(getW(i)).getValue(),   current->at(getE(i)).getValue(),
                 current->at(getSW(i)).getValue(),  current->at(getS(i)).getValue(), current->at(getSE(i)).getValue() };
      return arr;
    }
};

struct Worker: ff::ff_node_t<pair_t, int> {
  int n = 0;
  iFunctionCall rule;
  Table* table;
  long start, stop;

  Worker(iFunctionCall rule, Table* table): 
    rule(rule), table(table) {
      n = 0;
  }
  int* svc(pair_t* in) {
    /* cout << "received " << in->first << " " << in->second <<  endl; */ 
    if (in-> first != -1 && in->second != -1) {
      start = in->first;
      stop = in->second;
      /* cout << start << " " << stop << endl; */
    }
    /* cout << "setting table" << endl; */
    for (long i = start; i <= stop; i++) {
      int val = table->getCellValue(i);
      int nVal = rule(val, table->getNeighbours(i));
      table->setFuture(i, nVal);
      /* cout << "setting cell: " << i << " with value: " << nVal << endl; */
    }
    //n++;
    return &n;
  }
};

struct Emitter: ff::ff_monode_t<int, pair_t> {

  Table* table;
  int offset, remaining, threadsR, nw, nSteps;
  long start, stop, size;
  pair_t* pairs = nullptr; // vector of pairs, one for each Worker
  pair_t START_TASK = {-1, -1};
  Emitter(const int nSteps, const int nw, const long size, Table* table): 
    nSteps(nSteps), nw(nw), size(size), table(table) {
      pairs = new pair_t[nw];
  }

  ~Emitter() { if (pairs) delete [] pairs; }

  int svc_init() {
    offset    = size / nw;
    remaining = size % nw;
    start     = 0;
    stop      = offset - 1;
    threadsR  = 0;
    return 0;
  }

  pair_t* svc(int *s) {
    // first execution, split grid within workers
    if (s == nullptr) {
      for (int i = 0; i < nw; i++) {
        /* std::cout << "sending: " << start << " " << stop << "to thread :" << i << endl; */
        pairs[i] = {start, stop};
        ff_send_out_to(&pairs[i], i);
        start += offset;
        if (i == (nw - 2)) stop += offset + remaining;
        else stop += offset;
      }
      return GO_ON;
    }
    // if all threads have replied back
    else if (++threadsR == nw && nSteps > 1) {
      /* cout << "All threads ready for next step" << endl; */
      threadsR = 0;
      nSteps--;
      //table->printCurrent();
      table->swapCurrentFuture();
      broadcast_task(&START_TASK);
    } 
    else if (threadsR == nw && nSteps == 1) {
      /* cout << "All threads done" << endl; */
      //table->printCurrent();
      //table->printFuture();
      broadcast_task(EOS);
    }
    // else we still have to wait some threads
    return GO_ON;
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
    long size;

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

    Game(long height, long width, iFunctionCall rule, int nw, int nSteps):
      rule(rule), nw(nw), nSteps(nSteps) {
        table = Table(height, width);
        size = height * width;
    }
    
    int run() {

      if (nw == 1) {
        for (int j = 0; j < nSteps; j++) {
          for (int i = 0; i < size; i++) {
            int val = table.getCellValue(i);
            int nVal = rule(val, table.getNeighbours(i));
            table.setFuture(i, nVal);
          }
          table.swapCurrentFuture();
        }
        return 0;
      }

      Emitter E(nSteps, nw, size, &table);
      ff::ff_Farm<> farm( [&]() {
        std::vector<std::unique_ptr<ff::ff_node> > W;
        for(int i = 0; i < nw; i++) {
          W.push_back(ff::make_unique<Worker>(rule, &table));
          cout << "Worker created" << endl;
        }
        return W;
      } (),
      E);
      farm.remove_collector();
      farm.wrap_around();
      if (farm.run_and_wait_end() < 0) {
        ff::error("running farm");
        return -1;
      }
      cout << "SET" << endl;
      return 0;
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
  auto height   = atol(argv[1]);
  // width
  auto width    = atol(argv[2]);
  // number of workers
  auto nWorkers = atoi(argv[3]);
  // number of steps
  auto nSteps   = atoi(argv[4]);
  // rule to apply (default is game of life)
  iFunctionCall rule = gameRule;
  if (argc == 6) {
    rule = rules[atoi(argv[5])];
  }

  Game g = Game(height, width, rule, nWorkers, nSteps);
  ff::ffTime(ff::START_TIME);
  g.run();
  ff::ffTime(ff::STOP_TIME);
    std::cout << "Computed in: " << std::setprecision(8) << ff::ffTime(ff::GET_TIME) << " ms"  << std::endl;
  return 0;
}