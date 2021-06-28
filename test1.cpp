#include <iostream>
#include <cstdlib>
#include "framework1.0.hpp"

using namespace std;

class Test: public Game {
  public:
    Test(int height, int width, int nw, int nSteps) 
      : Game { height, width, nw, nSteps } {}

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
    cout << "Usage is " << argv[0] << " height width num_workers num_steps [rule_id]" << endl;
    return(-1);
  }

  // height
  auto height   = atoi(argv[1]);
  // width
  auto width    = atoi(argv[2]);
  // number of workers
  auto nWorkers = atoi(argv[3]);
  // number of steps
  auto nSteps   = atoi(argv[4]);

  // automaton setup
  Test g = Test(height, width, nWorkers, nSteps);
  // timing the run
  g.run();

  return 0;
}