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

/* Redefining vector of ints as row */
using row = std::vector<int>;

/* Redefining pair of vectors of ints*/
using pair_v = std::pair<vector<int>, vector<int>>;

/**
 * Class modeling the matrix
 * 
 * Contains the current state of the matrix and its future state on the next step
 */
class Table {
  private:
    vector<row>* current_rows = new vector<row>();
    vector<row>* future_rows = new vector<row>();
    long width;
    long height;
    long size;

  public:
    Table() {}

    // Constructor
    Table(long height, long width):
      height(height), width(width) {
      size = height * width;
    }

    // Constructor initializing the table with input values
    Table(long height, long width, vector<int> input):
      height(height), width(width) {
      size = height * width;

      for (long i = 0; i < height; i++) {
        current_rows->push_back(vector<int>());
        future_rows->push_back(vector<int>());
      }

      for (long i = 0; i < size; i++) {
        long column, row;
        row = i / width;
        column = i % width;
        current_rows->at(row).push_back(input[i]);
        future_rows->at(row).push_back(0);
      }
    }

    /**
     * Populate the matrix with random values cells
     */
    void generate() {
      for (long i = 0; i < height; i++) {
        current_rows->push_back(vector<int>());
        future_rows->push_back(vector<int>());
      }

      for (long i = 0; i < size; i++) {
        long column, row;
        row = i / width;
        column = i % width;
        current_rows->at(row).push_back(rand() % 2);
        future_rows->at(row).push_back(0);
      }
    }

    // Getters
    vector<row>* getCurrent() { return current_rows; }
    vector<row>* getFuture() { return future_rows; }

    int getCellValue(long row, long column) {
      return current_rows->at(row)[column];
    }

    int getCellValue(long i) {
      long column, row;
      row = i / width;
      column = i % width;
      return current_rows->at(row)[column];
    }

    long getSize() { return size; }
    long getHeight() { return height; }
    long getWidth() { return width; }

    row getRow(long row) {
      return current_rows->at(row);
    }

    // Setters
    void setFuture(long row, long column, int value) {
      future_rows->at(row)[column] = value;
    }

    void setCurrent(long row, long column, int value) {
      current_rows->at(row)[column] = value;
    }

    void setFuture(long i, int value) {
      long column, row;
      row = i / width;
      column = i % width;
      future_rows->at(row)[column] = value;
    }

    void setCurrent(long i, int value) {
      long column, row;
      row = i / width;
      column = i % width;
      current_rows->at(row)[column] = value;
    }

    /**
     * Prints the current state of the matrix
     */
    void printCurrent() {
      for (long i = 0; i < height; i++) {
        for (long j = 0; j < width; j++) {
          int v = current_rows->at(i).at(j);
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
      for (long i = 0; i < height; i++) {
        for (long j = 0; j < width; j++) {
          int v = future_rows->at(i).at(j);
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
      auto tmp = current_rows;
      current_rows = future_rows;
      future_rows = tmp;
    }

    /**
     * Retrieves the state of the neighbours of the given index
     * 
     * @param row row index of the cell in examination
     * @param column column index of the cell in examination
     * @returns a vector containing the 8 values of the cell's neighbourhood
     */
    vector<int> getNeighbours(long row, long column) {
      vector<int> arr = 
        {
        (*current_rows)[(mod(row - 1, height))][mod(column - 1, width)], (*current_rows)[(mod(row - 1, height))][column], (*current_rows)[(mod(row - 1, height))][mod(column + 1, width)],
        (*current_rows)[row][mod(column - 1, width)], (*current_rows)[row][mod(column + 1, width)],
        (*current_rows)[(mod(row + 1, height))][mod(column - 1, width)], (*current_rows)[(mod(row + 1, height))][column], (*current_rows)[(mod(row + 1, height))][mod(column + 1, width)],
        };
      return arr;
    }

    /**
     * Retrieves the state of the neighbours of the given index
     * 
     * @param i index of the cell in examination
     * @returns a vector containing the 8 values of the cell's neighbourhood
     */
    vector<int> getNeighbours(long i) {
      long column, row;
      row = i / width;
      column = i % width;
      vector<int> arr = 
        {
        (*current_rows)[(mod(row - 1, height))][mod(column - 1, width)], (*current_rows)[(mod(row - 1, height))][column], (*current_rows)[(mod(row - 1, height))][mod(column + 1, width)],
        (*current_rows)[row][mod(column - 1, width)], (*current_rows)[row][mod(column + 1, width)],
        (*current_rows)[(mod(row + 1, height))][mod(column - 1, width)], (*current_rows)[(mod(row + 1, height))][column], (*current_rows)[(mod(row + 1, height))][mod(column + 1, width)],
        };
      return arr;
    }
};