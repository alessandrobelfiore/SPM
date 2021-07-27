/**
 * This framework supports the execution of different rules on a cellular automata
 * represented using a 1D array. The parallelism is obtained using stdlib C++ threads
 * synchronized using mutexes and condition variables. The workload is divided
 * indipendently of rows.
 * 
 * To use the user should implement a subclass of Game, implement the virtual method rule,
 * instantiate an object of the class, and call the method run().
 */
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <cstdlib>
#include <chrono>

#include "ints_1D_t_ref.hpp"
//#include "cells_1D_t.hpp"

using namespace std;

// redefining clock from chrono library for easier use
typedef std::chrono::high_resolution_clock Clock;

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
    int size;
    // number of threads ready to get to the next step
    atomic<int> threadsReady;
    // number of threads completed
    atomic<int> threadsDone;
    // flag for synchronization via active wait
    atomic<int> currentStep;

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
    Game(int height, int width, int nw):
      nw(nw) {
        if (nw <= 0 || width <= 0 || height <= 0) {
          throw "Invalid parameters, check framework API";
        }
        table = Table(height, width);
        size = height * width;
        threadsReady = 0;
        threadsDone = 0;
    }

    // Constructor with initializiation of the matrix values
    Game(int height, int width, int nw, vector<int> input):
      nw(nw) {
        if (nw <= 0 || width <= 0 || height <= 0) {
          throw "Invalid parameters, check framework API";
        }
        table = Table(height, width, input);
        size = height * width;
        threadsReady = 0;
        threadsDone = 0;
        currentStep = -1;
    }

    /**
     * Function passed to each thread to compute the algorithm on the cells
     * 
     * @param start index of the matrix where the thread must start the computation
     * @param stop index of the matrix of the last cell assigned to the thread
     */
    void execute(int start, int stop) {
       vector<int>* neigh = new vector<int> (8);
        for (int j = 0; j < nSteps; j++) {
          for (int i = start; i <= stop; i++) {
            int val = table.getCellValue(i);
            table.getNeighboursRef(i, neigh);
            int nVal = rule(val, neigh);
            table.setFuture(i, nVal);
        }
        ++threadsReady;
        while (currentStep.load() != j) {}
      }
      //cout << "Thread: " << this_thread::get_id() << " is done" << endl;
      threadsDone++;
      return;
    }
    
    /**
     * Function containing the algorithm to use to compute the next state of a cell
     * 
     * @param val the value of the current state of the cell
     * @param arr an array containing the states of the neighbourhood of the cell
     * @returns the new state of the cell
     */
    virtual int rule(int val, vector<int>* arr) { return 0; };

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
     * @returns the overhead for parallel computation in milliseconds
     */
    double run(int steps) {
      nSteps = steps;

      auto startTime = Clock::now();
      
      if (nw == 1) {
        vector<int>* neigh = new vector<int> (8); 
        for (int j = 0; j < nSteps; j++) {
          for (int i = 0; i < size; i++) {
            int val = table.getCellValue(i);
            table.getNeighboursRef(i, neigh);
            int nVal = rule(val, neigh);
            table.setFuture(i, nVal);
          }
          table.swapCurrentFuture();
        }
        return 0;
      }

      vector<thread*> tids(nw);
      int offset = size / nw;
      int remaining  =  size % nw;
      int start = 0;
      int stop = offset - 1;
      for(int i = 0; i < nw; i++) {
        tids[i] = new thread(&Game::execute, this, start, stop);
        start += offset;
        if (i == (nw - 2)) stop += offset + remaining;
        else stop += offset;
      }

      auto endTime = Clock::now();
      auto setupTime = chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

      // if computation is not over
      while (threadsDone.load() != nw) {
        // check if threads are all ready for the next step
        while (threadsReady.load() < nw) {
          if (threadsDone.load() == nw) break;
        }
        if (threadsDone.load() == nw) break;
        startTime = Clock::now();
        table.swapCurrentFuture();
        threadsReady.exchange(0);
        currentStep++;
        endTime = Clock::now();
        setupTime = setupTime + chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();
      }

      startTime = Clock::now();

      threadsReady.exchange(0);
      threadsDone.exchange(0);

      for(auto e : tids)
        e->join();

      endTime = Clock::now();
      return setupTime + chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();
    }
};