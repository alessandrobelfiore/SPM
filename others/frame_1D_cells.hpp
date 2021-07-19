#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <cstdlib>
#include <chrono>

#include "../ints_1D_t.hpp"
// ints are slightly more performing
//#include "../cells_1D_t.hpp"

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
    // The matrix
    Table table;
    // Number of workers
    int nw;
    // Number of steps
    int nSteps;
    // Number of cells
    int size;
    // Wait here until nextStep is executable
    condition_variable nextStep;
    // Wait here until a thread has completed a step
    condition_variable check;
    // Utility mutexes
    mutex m;
    mutex m1;
    // Number of threads ready to get to the next step
    atomic<int> threadsReady;
    // Number of threads completed
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
    Game(int height, int width, int nw, int nSteps):
      nw(nw), nSteps(nSteps) {
        table = Table(height, width);
        size = height * width;
        threadsReady = 0;
        threadsDone = 0;
    }

    /**
     * Function passed to each thread to compute the algorithm on the cells
     * 
     * @param start index of the matrix where the thread must start the computation
     * @param stop index of the matrix of the last cell assigned to the thread
     */
    void execute(int start, int stop) {
      for (int j = 0; j < nSteps; j++) {
        for (int i = start; i <= stop; i++) {
          int val = table.getCellValue(i);
          int nVal = rule(val, table.getNeighbours(i));
          table.setFuture(i, nVal);
        }
        //cout << "Step: " << j << " ended" << endl;
        unique_lock<mutex> lock(m);
        if (++threadsReady == nw) {
          //cout << "Notifying main thread!" << endl;
          check.notify_all();
        }
        nextStep.wait(lock);
      }
      //cout << "Thread: " << this_thread::get_id() << " is done" << endl;
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
      
      if (nw == 1) {
        for (int j = 0; j < nSteps; j++) {
          for (int i = 0; i < size; i++) {
            int val = table.getCellValue(i);
            int nVal = rule(val, table.getNeighbours(i));
            table.setFuture(i, nVal);
          }
          //table.printCurrent();
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

      for(auto e : tids)
        e->join();

      auto endTime = Clock::now();
      return chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
    }
};
