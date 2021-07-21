#include <iostream>
#include <cstdlib>
#include <chrono>
#include <climits>

// Employ the 1D matrix implementation
#ifndef TWOD
#include "frame_threads_1D.hpp"
#endif
// Employ the 2D matrix implementation
#ifdef TWOD
#include "frame_threads_2D.hpp"
#endif


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

int main(int argc, char* argv[]) {
  // args
  if (argc != 5 && argc != 6) {
    cout << "Received " << argc - 1 << " of the minimum 4 arguments" << endl;
    cout << "Usage is " << argv[0] << " height width num_workers num_steps" << endl;
    return(-1);
  }

  // height
  auto height   = atol(argv[1]);
  if (height <= 0) {
    cout << "Height cannot be less than 1" << endl;
    cout << "Usage is " << argv[0] << " height width num_workers num_steps" << endl;
    return(-1);
  }

  // width
  auto width    = atol(argv[2]);
  if (width <= 0) {
    cout << "Width cannot be less than 1" << endl;
    cout << "Usage is " << argv[0] << " height width num_workers num_steps" << endl;
    return(-1);
  }

  // number of workers
  auto nWorkers = atoi(argv[3]);
  if (nWorkers <= 0) {
    cout << "Workers must be al least 1" << endl;
    cout << "Usage is " << argv[0] << " height width num_workers num_steps" << endl;
    return(-1);
  }

  // number of steps
  auto nSteps   = atoi(argv[4]);
  if (nSteps <= 0) {
    cout << "Number of steps must be > 0" << endl;
    cout << "Usage is " << argv[0] << " height width num_workers num_steps" << endl;
    return(-1);
  }

  // number of runs
  auto nRuns = 1;
  if (argc == 6) {
    nRuns  = atoi(argv[5]);
    if (nRuns <= 0) {
      cout << "Number of runs must be > 0" << endl;
      cout << "Usage is " << argv[0] << " height width num_workers num_steps" << endl;
      return(-1);
    }
  }

  vector<long> timings(nRuns);
  vector<long> computations(nRuns);
  long max = 0;
  long avg = 0;
  long avgC = 0;
  long sum = 0;
  long sumC = 0; 
  long min = LONG_MAX;
  srand(112233);

  vector<int> input (height * width);
  for (int i = 0; i < height * width; i++) {
    input[i] = rand() % 2;
  }

  // automaton setup
  try {
  Test g = Test(height, width, nWorkers, input);
  for (int i = 0; i < nRuns; i++) {
    // timing the run
    auto startTime = Clock::now();
    computations[i] = g.run(nSteps);
    auto endTime = Clock::now();
    timings[i] = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
    if (timings[i] > max) max = timings[i];
    if (timings[i] < min) min = timings[i];
    sum += timings[i];
    sumC += computations[i];
  }

  g.print();

  avg   = sum / nRuns;
  avgC  = sumC / nRuns;

  cout << "Minimum time: " << min << " ms" << endl;
  cout << "Maximum time: " << max << " ms" << endl;
  cout << "Average time: " << avg << " ms" << endl;
  cout << "Average time comp: " << avgC << " ms" << endl;
  } catch (const char* msg) {
    cerr << msg << endl;
  }

  return 0;
}