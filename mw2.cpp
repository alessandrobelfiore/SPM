#include <iostream>
#include <iomanip>
#include <vector>
#include <ff/ff.hpp>
#include <ff/farm.hpp>

using namespace std;
using namespace ff;

typedef int (* iFunctionCall)(int args, vector<int> arr);

using pair_t = std::pair<int, int>;

/* Redefining the module operator */
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

    bool operator== (Cell& rhs) {
      if (index == rhs.index && value == rhs.value) return true;
      else return false;
    }
};

/* redefining vectors of cells as rows */
using row = std::vector<Cell>;
/* redefining pairs of vectors of cells*/
using pair_v = std::pair<vector<Cell>, vector<Cell>>;

// parametric? FIXME later
pair_v make_start_task() {
  auto tmp = new vector<Cell>;
  tmp->push_back(Cell(0,-1,0,0));
  auto tmp1 = new vector<Cell>;
  tmp1->push_back(Cell(0,-1,0,0));
  return make_pair(*tmp, *tmp1);
}

auto START_TASK = make_start_task();

/* Class representing a table
  current_rows: vector of rows representing the current state of the table
  future_rows:  vector of rows representing the future state of the table
  width:        width of the table
  height:       height of the table
  size:         size of the table */
class Table {
  private:
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

    void setFuture(long i, int value) {
      long column, row;
      row = i / width;
      column = i % width;
      future_rows->at(row)[column].setValue(value);
    }

    void setCurrent(long i, int value) {
      long column, row;
      row = i / width;
      column = i % width;
      current_rows->at(row)[column].setValue(value);
    }

    int getCellValue(long row, long column) {
      return current_rows->at(row)[column].getValue();
    }
    int getCellValue(long i) {
      long column, row;
      row = i / width;
      column = i % width;
      return current_rows->at(row)[column].getValue();
    }
    
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

    pair_t getNW(long row, long column) { return {mod(row - 1, height), mod(column - 1, width)}; } 
    pair_t getN(long row, long column) { return {mod(row - 1, height), column}; } 
    pair_t getNE(long row, long column) { return {mod(row - 1, height), mod(column + 1, width)}; }
    pair_t getW(long row, long column) { return {row, mod(column - 1, width)}; }
    pair_t getE(long row, long column) { return {row, mod(column + 1, width)}; }
    pair_t getSW(long row, long column) { return {mod(row + 1, height), mod(column - 1, width)}; }
    pair_t getS(long row, long column) { return {mod(row + 1, height), column}; }
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
        {current_rows->at(row - 1)[column - 1].getValue(), current_rows->at(row - 1)[column].getValue(), current_rows->at(row - 1)[column + 1].getValue(),
        current_rows->at(row)[column - 1].getValue(), current_rows->at(row)[column + 1].getValue(),
        current_rows->at(row + 1)[column - 1].getValue(), current_rows->at(row + 1)[column].getValue(), current_rows->at(row + 1)[column + 1].getValue(),
        };
      return arr;
    }

    vector<int> getNeighbours(int i) {
      long column, row;
      row = i / width;
      column = i % width;
      vector<int> arr = 
                {current_rows->at(getNW(row, column).first).at(getNW(row, column).second).getValue(),  current_rows->at(getN(row, column).first).at(getN(row, column).second).getValue(), current_rows->at(getNE(row, column).first).at(getNE(row, column).second).getValue(), 
                 current_rows->at(getW(row, column).first).at(getW(row, column).second).getValue(),   current_rows->at(getE(row, column).first).at(getE(row, column).second).getValue(),
                 current_rows->at(getSW(row, column).first).at(getSW(row, column).second).getValue(),  current_rows->at(getS(row, column).first).at(getS(row, column).second).getValue(), current_rows->at(getSE(row, column).first).at(getSE(row, column).second).getValue() };
      return arr;
    }
};

/* Class representing a worker node
  id:     worker id
  rule:   rule to apply to get the next state of the cell
  table:  reference to the table(subtable) to work on
  start:  starting index of the cells to compute
  stop:   last index of the cells to compute */
struct Worker: ff_node_t<pair_v, int> {
  int id;
  iFunctionCall rule;
  Table* table;
  long start, stop;

  Worker(iFunctionCall rule, Table* table, int id): 
    rule(rule), table(table), id(id) {
      start = table->getWidth();
      stop = table->getSize() - table->getWidth();
      cout << "start " << start << " stop " << stop << endl;
    }

  int* svc(pair_v* in) {
    // in = pair of vector<Cell> representing top and bot ghost rows
    // receive ghost rows from emitter
    // and set
    if (in != &START_TASK) {
      for (long j = 0; j < table->getWidth(); j++) {
        table->setCurrent(0, j, in->first[j].getValue());
        table->setCurrent(table->getHeight() - 1, j, in->second[j].getValue());
      }
      /* table->printCurrent(); */
      return GO_ON;
    }
    for (long i = start; i < stop; i++) {
      int val = table->getCellValue(i);
      int nVal = rule(val, table->getNeighbours(i));
      table->setFuture(i, nVal);
      /* cout << "setting cell: " << i << " with value: " << nVal << endl; */
    }
    /* cout << "setting future" << endl; */
    table->swapCurrentFuture();
    /* cout << "future swapped in thread: " << id << endl; */
    return &id;
  }
};

/* Class representing the emitter node
  table:      reference to the table
  subtables:  reference to the array of subtables composing the original table
  pairs:      reference to the vector of pairs of rows to be sent to each worker
  threads_r:  number representing the amount of workers
  nw:         number of workers
  nSteps:     number of steps */
struct Emitter: ff_monode_t<int, pair_v> {

  Table* table;
  vector<Table>* subtables;
  vector<pair_v>* pairs;
  int threads_r, nw, nSteps;
  Emitter(const int nSteps, const int nw, const long size, Table* table, vector<Table>* subtables): 
    nSteps(nSteps), nw(nw), table(table), subtables(subtables) {
      threads_r = 0;
      pairs = new vector<pair_v>(nw);
    }

  pair_v* svc(int *s) {

    if (s == nullptr) {
      broadcast_task(&START_TASK);
      nSteps --;
      return GO_ON;
    }
    else {
      // receive thread ready id
      // send to neighbouring thread
      /* cout << "received: " << *s << endl; */
      threads_r ++;

      if (nSteps <= 0) {
        broadcast_task(EOS);
      }
      else {
        /* if (threadsR->at(mod(*s - 2, nw)) == true && threadsR->at(mod(*s - 1, nw)) == true) {
          auto tmp = subtables->at(*s);
          auto tmp2 = subtables->at(mod(*s - 2, nw));
          auto pair = {tmp2.getRow(tmp.getHeight() - 2), tmp.getRow(1)};
          ff_send_out(&pair, mod(*s - 1, nw));
          cout << "sent rows to thread: " << mod(*s - 1, nw) << endl;
          threadsR->at(mod(*s - 1, nw)) = false;
        }

        if (threadsR->at(mod(*s + 2, nw)) == true &&  threadsR->at(mod(*s + 1, nw)) == true) {
          auto tmp = subtables->at(*s);
          auto tmp2 = subtables->at(mod(*s + 2, nw));
          auto pair = {tmp.getRow(tmp.getHeight() - 2), tmp2.getRow(1)};
          ff_send_out(&pair, mod(*s + 1, nw));
          cout << "sent rows to thread: " << mod(*s + 1, nw) << endl;
          threadsR->at(mod(*s + 1, nw)) = false;
        }

        if (threadsR->at(mod(*s + 1, nw)) == true &&  threadsR->at(mod(*s - 1, nw)) == true) {
          auto tmp = subtables->at(mod(*s + 1, nw));
          auto tmp2 = subtables->at(mod(*s - 1, nw));
          auto pair = {tmp2.getRow(tmp.getHeight() - 2), tmp.getRow(1)};
          ff_send_out(&pair, *s);
          cout << "sent rows to thread: " << *s << endl;
          threadsR->at(*s) = false;
        } */
        // this section should be locked
        if (threads_r == nw) {
          pairs->clear();
          for (int i = 0; i < nw; i++) {
            
            /* subtables->at(i).printFuture(); */
            /* cout << "trying to send" << endl; */
            /* cout << "MOD CHECK KEKW: " << mod(i + 1, nw) << " -- " << mod(i - 1, nw) << endl; */
            auto tmp = subtables->at(mod(i + 1, nw));
            auto tmp2 = subtables->at(mod(i - 1, nw));
            auto pair = make_pair(tmp2.getRow(tmp2.getHeight() - 2), tmp.getRow(1));
            pairs->push_back(pair);
            ff_send_out(&pairs->at(i), i);  
            /* cout << "sent rows to thread: " << i << endl; */
          }
          threads_r = 0;
          nSteps --;
          broadcast_task(&START_TASK);
        }

      }
      return GO_ON;
    }
  }
};

/* Class representing a game instance
  table:      the table
  subtables:  array of subtables composing the original table
  rule:       rule to apply to get the next state of the cell
  nw:         number of workers
  nSteps:     number of steps to execute
  size:       size of the table */
class Game {
  private:
    // game table
    Table table;
    vector<Table> subtables;
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
      subtables = obj.subtables;
      table = obj.table;
      rule = obj.rule;
      nw = obj.nw;
      nSteps = obj.nSteps;
      size = obj.size;
    }
    Game& operator=(const Game&& obj) //move constructor (must be explicitly declared if class has non-copyable member)
    {
      subtables = obj.subtables;
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
        table.generate();
        size = height * width;
        long s_height_avg = height / nw; // avg height of the subtables
        long s_height = s_height_avg;   // actual height of the current subtable (including the last one)
        long s_width = width;
        long remaining  = height % nw;

        table.printCurrent();

        if (nw != 1) {
          for (int i = 0; i < nw; i++) {
            if (i == (nw - 1)) s_height += remaining;
            subtables.push_back(Table(s_height + 2, s_width)); // TODO try shallow copy
            // starting from one row above and one below to include ghost rows
            for (int j = -1; j < s_height + 1; j++) {
              subtables[i].getCurrent()->push_back(table.getRow(mod(j + (i * s_height_avg), height)));
              subtables[i].getFuture()->push_back(table.getRow(mod(j + (i * s_height_avg), height)));
            }
            subtables[i].printCurrent();
          }
        }
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

      Emitter E(nSteps, nw, size, &table, &subtables);
      ff_Farm<> farm( [&]() {
        std::vector<std::unique_ptr<ff_node> > W;
        for(int i = 0; i < nw; i++) {
          W.push_back(ff::make_unique<Worker>(rule, &subtables[i], i));
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
      for (int i = 0; i < nw; i++) {
        subtables[i].printCurrent();
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

  #if 0
  ff::ffTime(ff::START_TIME);
  #endif
  Game g = Game(height, width, rule, nWorkers, nSteps);
  #if 1
  ff::ffTime(ff::START_TIME);
  #endif
  g.run();
  ff::ffTime(ff::STOP_TIME);
    std::cout << "Computed in: " << std::setprecision(8) << ff::ffTime(ff::GET_TIME) << " ms"  << std::endl;
  return 0;
}