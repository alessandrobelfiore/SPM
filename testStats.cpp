#include <iostream>
#include <cstdlib>
#include <chrono>
#include <climits>

#include "stats.hpp"
using namespace std;

int main(int argc, char* argv[]) {
  // args
  if (argc != 4 && argc != 5) {
    cout << "Received " << argc - 1 << " of the minimum 4 arguments" << endl;
    cout << "Usage is " << argv[0] << " height width num_workers num_steps num_runs" << endl;
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
  auto max_workers = atoi(argv[3]);
  if (max_workers <= 0) {
    cout << "Workers must be al least 1" << endl;
    cout << "Usage is " << argv[0] << " height width num_workers num_steps" << endl;
    return(-1);
  }

  srand(112233);
  vector<int> input (height * width);
  for (int i = 0; i < height * width; i++) {
    input[i] = rand() % 2;
  }

  Benchmark b = Benchmark(height, width, input, max_workers);
  b.bench();
  return 0;
}