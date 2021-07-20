/**
 * This framework supports the execution of different rules on a cellular automata
 * represented using a 2D matrix. The parallelism is obtained using stdlib C++ threads
 * synchronized using mutexes and condition variables. The workload is divided only by rows
 */
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <cstdlib>
#include <chrono>

#include "ints_2D_t.hpp"
//#include "cells_2D_t.hpp"

// redefining clock from chrono library for easier use
typedef std::chrono::high_resolution_clock Clock;

using namespace std;

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
      height(height), width(width), nw(nw) {
        table = Table(height, width);
        table.generate();
        size = height * width;
        threadsReady = 0;
        threadsDone = 0;
    }

    Game(int height, int width, int nw, vector<int> input):
      height(height), width(width), nw(nw) {
        table = Table(height, width, input);
        size = height * width;
        threadsReady = 0;
        threadsDone = 0;
    }

    /**
     * Function passed to each thread to compute the algorithm on the cells
     * 
     * @param rows_start index of the first row assigned to this thread
     * @param rows_stop index of the last row assigned to this thread
     * @param width the number of cells in each row
     */
    void execute(int rows_start, int rows_stop, int width) {
      for (int j = 0; j < nSteps; j++) {
        for (int i = rows_start; i < rows_stop; i++) {
          for (int j = 0; j < width; j++) {
            int val = table.getCellValue(i, j);
            int nVal = rule(val, table.getNeighbours(i, j));
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
      if (threadsDone.load() == nw) { check.notify_all(); }
      return;
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
     * Starts the computation of the automata
     * 
     * @param steps number of steps to be performed
     * @returns the time elapsed in milliseconds
     */
    double run(int steps) {
      nSteps = steps;

      if (nw == 1) {
        for (int j = 0; j < nSteps; j++) {
          for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
              int val = table.getCellValue(i, j);
              int nVal = rule(val, table.getNeighbours(i, j));
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
        while (threadsReady.load() < nw) {
          //cout << threadsReady.load() << "=/=" << nw << endl;
          check.wait(lock);
          /* cout << "Woken up, tR: " << threadsReady.load() << "tD: " << threadsDone.load() << endl; */
          if (threadsDone.load() == nw) break;
        }
        if (threadsDone.load() == nw) break;
        table.swapCurrentFuture();
        //table.printCurrent();
        threadsReady.exchange(0);
        // send wake up signals
        nextStep.notify_all();
      }

      threadsReady.exchange(0);
      threadsDone.exchange(0);

      for(auto e : tids)
        e->join();

      auto endTime = Clock::now();
      return chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
    }
};