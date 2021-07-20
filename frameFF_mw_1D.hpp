/**
 * This framework supports the execution of different rules on a cellular automata
 * represented using a 1D array. The parallelism is obtained using FF basic blocks
 * to achieve a Master-Worker pattern.
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

//#include "cells_1D_t.hpp"
// ints are slightly more performing
#include "ints_1D_t.hpp"

// redefining clock from chrono library for easier use
typedef std::chrono::high_resolution_clock Clock;

/* Redefining pairs of ints*/
using pair_t = std::pair<int,int>;

using namespace std;
using namespace ff;

/**
 * Class representing the worker
 * 
 * @tparam the object from which we get the implemented rule to apply
 */
template<class C>
struct Worker: ff_node_t<pair_t, int> {
  int n = 0; // n is just a placeholder
  C* c; // instance of the class from which we get the rule method
  Table* table; // reference to the table
  long start, stop;

  // Constructor
  Worker(C* c, Table* table): 
    c(c), table(table) {
      n = 0;
  }

  int* svc(pair_t* in) {
    // First task received contains a pair<int> (start, stop) 
    if (in-> first != -1 && in->second != -1) {
      start = in->first;
      stop = in->second;
      /* cout << start << " " << stop << endl; */
    }
    // Subsequent tasks contain (-1, -1) and are used for synchronization
    for (long i = start; i <= stop; i++) {
      int val = table->getCellValue(i);
      int nVal = (c->rule)(val, table->getNeighbours(i));
      table->setFuture(i, nVal);
      /* cout << "setting cell: " << i << " with value: " << nVal << endl; */
    }
    return &n;
  }
};

/**
 * Class representing the emitter / master
 */
struct Emitter: ff_monode_t<int, pair_t> {

  Table* table; // reference to the table
  int offset, remaining, threadsR, nw, nSteps;
  long start, stop, size;
  pair_t* pairs = nullptr; // vector of pairs, one for each Worker
  pair_t START_TASK = {-1, -1}; // placeholder value to signal the workers to start working
  
  // Constructor
  Emitter(const int nSteps, const int nw, const long size, Table* table): 
    nSteps(nSteps), nw(nw), size(size), table(table) {
      pairs = new pair_t[nw];
  }

  // Destructor
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
    // If this is the first execution, we split the work between the workers 
    if (s == nullptr) {
      for (int i = 0; i < nw; i++) {
        pairs[i] = {start, stop};
        ff_send_out_to(&pairs[i], i); // send to worker i, the pair of indexes
        start += offset;
        if (i == (nw - 2)) stop += offset + remaining; // fix for divisible dimensions
        else stop += offset;
      }
      return GO_ON;
    }
    // If all threads have replied back
    else if (++threadsR == nw && nSteps > 1) {
      threadsR = 0;
      nSteps--;
      table->swapCurrentFuture();
      broadcast_task(&START_TASK);
    } 
    else if (threadsR == nw && nSteps == 1) {
      broadcast_task(EOS);
    }
    // Else we still have to wait some threads
    return GO_ON;
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
    // number of workers
    int nw;
    // number of steps
    int nSteps;
    // number of Cells
    long size;

  public:
    // Default constructor
    Game() {
    }
    // Copy constructor (must be explicitly declared if class has non-copyable member)
    Game(const Game& obj) 
    {
      table = obj.table;
      nw = obj.nw;
      nSteps = obj.nSteps;
      size = obj.size;
    }
    Game& operator=(const Game&& obj) // Move constructor (must be explicitly declared if class has non-copyable member)
    {
      table = obj.table;
      nw = obj.nw;
      nSteps = obj.nSteps;
      size = obj.size;
      return *this;
    }

    // Constructor
    Game(long height, long width, int nw):
      nw(nw) {
      table = Table(height, width);
      size = height * width;
    }

    // Constructor with initialization of the matrix values
    Game(long height, long width, int nw, vector<int> input):
      nw(nw) {
      table = Table(height, width, input);
      size = height * width;
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
      table.printCurrent();
    }
    
    /**
     * Starts the computation of the automata
     * 
     * @param steps number of steps to be performed
     * @returns the time elapsed in milliseconds
     */
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

      Emitter E(nSteps, nw, size, &table); // creating the emitter
      ff::ff_Farm<> farm( [&]() {
        vector<std::unique_ptr<ff_node> > W;
        for(int i = 0; i < nw; i++) {
          W.push_back(ff::make_unique<Worker<Game>>(this, &table)); // creating the workers
        }
        return W;
      } (),
      E);
      farm.remove_collector(); 
      farm.wrap_around(); // wrap around to create a MW pattern

      auto startTime = Clock::now();

      #ifdef NOMAP
      farm.no_mapping();
      #endif
      if (farm.run_and_wait_end() < 0) {
        ff::error("running farm");
        return -1;
      }

      auto endTime = Clock::now();
      return chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
    }
};