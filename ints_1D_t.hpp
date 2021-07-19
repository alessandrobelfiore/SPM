#include <iostream>
#include <cstdlib>
#include <vector>

using namespace std;

/**
 * Redefining of the modulo operation to properly work on negative values
 * 
 * @param a left-handside of the modulo operation
 * @param b right-handside of the modulo operation
 * @returns the modulo operation from a and b
 */
int mod(int a, int b) {
  int r = a - (int) (a / b) * b;
  return r < 0 ? (r + b) : r;
}

/**
 * Class modeling the matrix
 * 
 * Contains the current state of the matrix and its future state on the next step
 */
class Table {
  private:
    int* current;
    int* future;
    int width;
    int height;

  public:
    // Default constructor
    Table() {}

    // Constructor initializing the table with random values
    Table(int height, int width):
      height(height), width(width) {
      int size = height * width;
      int column, row;
      current = new int[size];
      future = new int[size];
      for (int i = 0; i < size; i++) {
        row = i / width;
        column = i % width;
        current[i] = rand() % 2;
        future[i] = 0;
      }
    }

    // Constructor initializing the table with input values
    Table(int height, int width, vector<int> input):
      height(height), width(width) {
      int size = height * width;
      int column, row;
      current = new int[size];
      future = new int[size];
      for (int i = 0; i < size; i++) {
        row = i / width;
        column = i % width;
        current[i] = input[i];
        future[i] = 0;
      }
    }

    // Getters
    int* getCurrent() { return current; }

    int getCellValue(int i) {
      return current[i];
    }

    // Setter
    void setFuture(int index, int value) {
      future[index] = value;
    }


    /**
     * Prints the current state of the matrix
     */
    void printCurrent() {
      for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
          int v = current[i * width + j];
          if (v == 0) cout << "-";
          else cout << "x";
        }
        cout << endl;
      }
      cout << endl;
    }

    /**
     * Prints the next state of the matrix
     */
    void printFuture() {
      for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
          int v = future[i * width + j];
          if (v == 0) cout << "-";
          else cout << "x";
        }
        cout << endl;
      }
      cout << endl;
    }

    /**
     * Swaps the next state of the matrix with the current one
     */
    void swapCurrentFuture() {
      auto tmp = current;
      current = future;
      future = tmp;
      // TODO
      // std::swap(current, future);
    }

    /**
     * Retrieves the state of the neighbours of the given index
     * 
     * @param i index of the cell in examination
     * @returns a vector containing the 8 values of the cell's neighbourhood
     */
    vector<int> getNeighbours(int i) {
      long column, row;
      row = i / width;
      column = i % width;
      long x1 = width * (mod(row - 1, width));
      long x2 = width * (mod(row + 1, width));
      long x3 = width * row;
      long y1 = mod(column - 1, height);
      long y2 = mod(column + 1, height);
      vector<int> arr = 
                {
                current[x1 + y1],  
                current[x1 + column], 
                current[x1 + y2], 
                current[x3 + y1],   
                current[x3 + y2],
                current[x2 + y1],
                current[x2 + column],
                current[x2 + y2],
                };
      return arr;
    }
};