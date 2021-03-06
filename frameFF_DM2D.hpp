/**
 * This framework supports the execution of different rules on a cellular automata
 * represented using a 2D matrix. The parallelism is obtained using FF basic blocks
 * to achieve a Master-Worker pattern in which each worker has access to a local sub-matrix.
 * Between each step the Emitter sends to each worker the "phantom rows" necessary to 
 * continue the computation.
 * 
 * To use the user should implement a subclass of Game, implement the virtual method rule,
 * instantiate an object of the class, and call the method run().
 */
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <ff/ff.hpp>
#include <ff/farm.hpp>

#include "ints_2D_t.hpp"
//#include "cells_2D_t.hpp" // not working atm but tested to be inferior

typedef std::chrono::high_resolution_clock Clock;

using namespace std;
using namespace ff;

/* Redefining pair of vectors of ints*/
using pair_v = std::pair<vector<int>, vector<int>>;

/**
 * Utility function to create a token compatible with the workers,
 * used to start the workers but disregarded in value
 */
pair_v make_start_task() {
  auto tmp = new vector<int>;
  tmp->push_back(-1);
  auto tmp1 = new vector<int>;
  tmp1->push_back(-1);
  return make_pair(*tmp, *tmp1);
}

auto START_TASK = make_start_task();

/**
 * Class representing the worker
 * 
 * @tparam the object from which we get the implemented rule to apply
 */
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
      return GO_ON;
    }
    for (long i = start; i < stop; i++) {
      int val = table->getCellValue(i);
      int nVal = (g->rule)(val, table->getNeighbours(i));
      table->setFuture(i, nVal);
      /* cout << "setting cell: " << i << " with value: " << nVal << endl; */
    }
    table->swapCurrentFuture();
    /* cout << "future swapped in thread: " << id << endl; */
    return &id;
  }
};

/**
 * Class representing the emitter / master
 */
struct Emitter: ff_monode_t<int, pair_v> {

  vector<Table>* subtables;
  vector<pair_v>* pairs;
  int threads_r, nw, nSteps;

  // Constructor
  Emitter(const int nSteps, const int nw, const long size, vector<Table>* subtables): 
    nSteps(nSteps), nw(nw), subtables(subtables) {
      threads_r = 0;
      pairs = new vector<pair_v>(nw);
    }

  pair_v* svc(int *s) {

    // At the first execution, start all workers
    if (s == nullptr) {
      broadcast_task(&START_TASK);
      nSteps --;
      return GO_ON;
    }
    else {
      threads_r ++;
      // If the computation is over
      if (nSteps <= 0) {
        pairs->clear();
        // Synchronize phantom rows for last time
          for (int i = 0; i < nw; i++) {
            auto tmp = subtables->at(mod(i + 1, nw));
            auto tmp2 = subtables->at(mod(i - 1, nw));
            auto pair = make_pair(tmp2.getRow(tmp2.getHeight() - 2), tmp.getRow(1));
            pairs->push_back(pair);
            ff_send_out(&pairs->at(i), i);  
          }
        // And broadcast EOS
        broadcast_task(EOS);
      }
      else {
        // If all threads are in ready state
        if (threads_r == nw) {
          pairs->clear();
          // For each worker, send to it the phantom rows it needs
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

/**
 * Class representing the main access point to the framework
 * 
 * Contains a Table, a method rule, and the logic necessary to compute the rule on all cells
 * of the table in parallel
 */
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
    long width;

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

    // Constructor initializing table with random values
    Game(long height, long width, int nw):
        nw(nw), width(width) {
        if (nw <= 0 || width <= 0 || height <= 0) {
          throw "Invalid parameters, check framework API";
        }
        table = Table(height, width);
        table.generate();
        size = height * width;
        long s_height_avg = height / nw; // avg height of the subtables
        long s_height = s_height_avg;   // actual height of the current subtable (including the last one)
        long s_width = width;
        long remaining  = height % nw;

        if (nw != 1) {
          for (int i = 0; i < nw; i++) {
            if (i == (nw - 1)) s_height += remaining;
            subtables.push_back(Table(s_height + 2, s_width));
            // starting from one row above and one below to include ghost rows
            for (int j = -1; j < s_height + 1; j++) {
              subtables[i].getCurrent()->push_back(table.getRow(mod(j + (i * s_height_avg), height)));
              subtables[i].getFuture()->push_back(table.getRow(mod(j + (i * s_height_avg), height)));
            }
            /* subtables[i].printCurrent(); */
          }
        }
    }

    // Constructor initializing table with input vector
    Game(long height, long width, int nw, vector<int> input):
        nw(nw), width(width) {
        if (nw <= 0 || width <= 0 || height <= 0) {
          throw "Invalid parameters, check framework API";
        }
        table = Table(height, width, input);
        size = height * width;
        long s_height_avg = height / nw; // avg height of the subtables
        long s_height = s_height_avg;   // actual height of the current subtable (including the last one)
        long s_width = width;
        long remaining  = height % nw;

        if (nw != 1) {
          for (int i = 0; i < nw; i++) {
            if (i == (nw - 1)) s_height += remaining;
            subtables.push_back(Table(s_height + 2, s_width));
            // starting from one row above and one below to include ghost rows
            for (int j = -1; j < s_height + 1; j++) {
              subtables[i].getCurrent()->push_back(table.getRow(mod(j + (i * s_height_avg), height)));
              subtables[i].getFuture()->push_back(table.getRow(mod(j + (i * s_height_avg), height)));
            }
            /* subtables[i].printCurrent(); */
          }
        }
    }

    /**
     * Function containing the algorithm to use to compute the next state of a cell
     * 
     * @param val the value of the current state of the cell
     * @param arr an array containing the states of the neighbourhood of the cell
     * @returns the new state of the cell
     */
    virtual int rule(int val, vector<int> arr) { return 0; };

    /**
     * Prints the current state of the automata
     */
    void print() {
      if (nw == 1) {
        table.printCurrent();
        return;
      }
      for (int i = 0; i < nw; i++) {
        for (long k = 1; k < subtables[i].getHeight() - 1; k++) {
          for (long j = 0; j < width; j++) {
            auto t = subtables[i].getCurrent();
            int v = (*t)[k][j];
            if (v == 0) cout << "-";
            else cout << "x";
          }
          cout << endl;
        }
        cout << endl;
      }
    }
    
    /**
     * Starts the computation of the automata
     * 
     * @param steps number of steps to be performed
     * @returns the time elapsed in milliseconds
     */
    double run(int steps) {
      nSteps = steps;
      if (steps == 0) return 0;

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

      Emitter E(nSteps, nw, size, &subtables);
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
    }
};