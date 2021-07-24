#include <iostream>
#include <chrono>
#include <vector>

//#include "frame_threads_1D.hpp"
//#include "frame_threads_2D.hpp"
#include "frameFF_mw_1D.hpp"
//#include "frameFF_DM2D.hpp"

using namespace std;
typedef std::chrono::high_resolution_clock Clock;

class Test: public Game {
  public:
    Test(int height, int width, int nw, vector<int> input) 
      : Game { height, width, nw, input } {}

    Test(int height, int width, int nw) 
      : Game { height, width, nw } {}

    int rule(int value, vector<int> neighValues) {
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
};

class Benchmark {
  private:
    long width;
    long height;
    long size;
    int max_nw;
    long seq_time;
    vector<Test*> apps;
    vector<double> timings;
    vector<double> overheads;
    
  public:
    Benchmark(long height, long width, vector<int> input, int max_nw): 
      height(height), width(width), max_nw(max_nw) {
        for (int nw = 1; nw <= max_nw; nw *= 2) {
          try {
            Test* t = new Test(height, width, nw, input);
            apps.push_back(t);
          } catch (const char* msg) {
            cerr << msg << endl;
          }
        }
    }

  void bench() {
    seq_time = 0;
    auto startTime = Clock::now();
    apps[0]->run(1000);
    auto endTime = Clock::now();
    seq_time = chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();
    overheads.push_back(0);
    timings.push_back(seq_time);
    double over = 0;

    for (int i = 1; i < apps.size(); i++) {
      startTime = Clock::now();
      over = apps[i]->run(1000);
      overheads.push_back(over);
      endTime = Clock::now();
      timings.push_back(chrono::duration_cast<chrono::microseconds>(endTime - startTime).count());
    }

    cout << "Application running 1000 steps on matrix " << height << "x" << width << endl << endl;
    cout << "Sequential time:                         " << seq_time << " us" << endl;
    int n_cores = 1;
    for (int i = 1; i < apps.size(); i++) {
      double serial_f = overheads[i] / timings[i];
      cout << "Application running on " << (n_cores *= 2) << " cores:          " << timings[i] << " us" << endl;
      cout << "Overhead is:                             " << overheads[i] << " us" << endl;
      cout << "Serial fraction is:                      " << serial_f << endl;
      cout << "Efficiency is:                           " << (seq_time / timings[i]) / n_cores << endl;
      cout << "Speedup is:                              " << seq_time / timings[i] << endl;
      delete(apps[i]);
    }
  }
};