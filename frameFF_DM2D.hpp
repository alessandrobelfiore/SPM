#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <ff/ff.hpp>
#include <ff/farm.hpp>

#include "ints_2D.hpp"
//#include "cells_2D_t.hpp" // not working atm but tested to be inferior

typedef std::chrono::high_resolution_clock Clock;

using namespace std;
using namespace ff;

/* Redefining pair of vectors of ints*/
using pair_v = std::pair<vector<int>, vector<int>>;

pair_v make_start_task() {
  auto tmp = new vector<int>;
  tmp->push_back(-1);
  auto tmp1 = new vector<int>;
  tmp1->push_back(-1);
  return make_pair(*tmp, *tmp1);
}

auto START_TASK = make_start_task();

/* Class representing a worker node
  id:     worker id
  rule:   rule to apply to get the next state of the cell
  table:  reference to the table(subtable) to work on
  start:  starting index of the cells to compute
  stop:   last index of the cells to compute */
template<class C>
struct Worker: ff_node_t<pair_v, int> {
  int id;
  C* g;
  Table* table;
  long start, stop;
  
  Worker(C* g, Table* table, int id): 
    g(g), table(table), id(id) {
      start = table->getWidth();
      stop = table->getSize() - table->getWidth();
     /*  cout << "start " << start << " stop " << stop << endl; */
    }

  int* svc(pair_v* in) {
    // in = pair of vector<int> representing top and bot ghost rows
    // receive ghost rows from emitter
    // and set
    if (in != &START_TASK) {
      for (long j = 0; j < table->getWidth(); j++) {
        table->setCurrent(0, j, in->first[j]);
        table->setCurrent(table->getHeight() - 1, j, in->second[j]);
      }
      /* table->printCurrent(); */
      return GO_ON;
    }
    for (long i = start; i < stop; i++) {
      int val = table->getCellValue(i);
      int nVal = (g->rule)(val, table->getNeighbours(i));
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
        // this section should be locked
        if (threads_r == nw) {
          pairs->clear();
          for (int i = 0; i < nw; i++) {
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
      nw = obj.nw;
      nSteps = obj.nSteps;
      size = obj.size;
    }
    Game& operator=(const Game&& obj) //move constructor (must be explicitly declared if class has non-copyable member)
    {
      subtables = obj.subtables;
      table = obj.table;
      nw = obj.nw;
      nSteps = obj.nSteps;
      size = obj.size;
      return *this;
    }

    Game(long height, long width, int nw):
        nw(nw) {
        table = Table(height, width);
        table.generate();
        size = height * width;
        long s_height_avg = height / nw; // avg height of the subtables
        long s_height = s_height_avg;   // actual height of the current subtable (including the last one)
        long s_width = width;
        long remaining  = height % nw;

        /* table.printCurrent(); */

        if (nw != 1) {
          for (int i = 0; i < nw; i++) {
            if (i == (nw - 1)) s_height += remaining;
            subtables.push_back(Table(s_height + 2, s_width)); // TODO try shallow copy
            // starting from one row above and one below to include ghost rows
            for (int j = -1; j < s_height + 1; j++) {
              subtables[i].getCurrent()->push_back(table.getRow(mod(j + (i * s_height_avg), height)));
              subtables[i].getFuture()->push_back(table.getRow(mod(j + (i * s_height_avg), height)));
            }
            /* subtables[i].printCurrent(); */
          }
        }
    }

    virtual int rule(int val, vector<int> arr) { return 0; };
    
    double run(int steps) {
      nSteps = steps;

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
      // TODO try lambda
      // auto lam = [&](int float) { rule(); }
      ff::ff_Farm<> farm( [&]() {
        vector<std::unique_ptr<ff_node> > W;
        for(int i = 0; i < nw; i++) {
          W.push_back(ff::make_unique<Worker<Game>>(this, &subtables[i], i));
        }
        return W;
      } (),
      E);
      farm.remove_collector();
      farm.wrap_around();
      auto startTime = Clock::now();

      if (farm.run_and_wait_end() < 0) {
        ff::error("running farm");
        return -1;
      }

      auto endTime = Clock::now();
      return chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
      /* for (int i = 0; i < nw; i++) {
        subtables[i].printCurrent();
        subtables[i].printFuture();
      } */
    }
};