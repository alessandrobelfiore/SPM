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
 * Class modeling a cell of the matrix
 * 
 * Contains the index in the 1D matrix, the original row and column indexes in the 
 * 2D matrix, and the value of the state
 */
class Cell {
  private: 
    int index;
    int row;
    int column;
    int value;
    
  public:
    // Default constructor
    Cell() = default;

    // Constructor
    Cell(int index, int value, int row, int column): 
      index(index), value(value), row(row), column(column) {}

    // Constructor without value
    Cell(int index, int row, int column):
      index(index), row(row), column(column) {}

    // Getters
    int getIndex() { return index; }
    int getValue() { return value; }
    int getRow() { return row; }
    int getColumn() { return column; }

    // Setter
    void setValue(int v) { value = v; }
};

/**
 * Class modeling the matrix
 * 
 * Contains the current state of the matrix and its future state on the next step
 */
class Table {
  private:
    Cell* current;
    Cell* future;
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
      current = new Cell[size];
      future = new Cell[size];
      for (int i = 0; i < size; i++) {
        row = i / width;
        column = i % width;
        current[i] = Cell(i, rand() % 2, row, column);
        future[i] = Cell(i, 0, row, column);
      }
    }

    // Getters
    Cell* getCurrent() { return current; }

    int getCellValue(int i) {
      return current[i].getValue();
    }

    // Setter
    void setFuture(int index, int value) {
      future[index].setValue(value);
    }


    /**
     * Prints the current state of the matrix
     */
    void printCurrent() {
      for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
          int v = current[i * width + j].getValue();
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
          int v = future[i * width + j].getValue();
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
      // std::swap(current, future);
    }

    /**
     * Retrieves the state of the neighbours of the given index
     * 
     * @param i index of the cell in examination
     * @returns a vector containing the 8 values of the cell's neighbourhood
     */
    vector<int> getNeighbours(int i) {   
      long x1 = width * (mod(current[i].getRow() - 1, width));
      long x2 = width * (mod(current[i].getRow() + 1, width));
      long x3 = width * (mod(current[i].getRow(), width));
      long y1 = mod(current[i].getColumn() - 1, height);
      long y2 = mod(current[i].getColumn() + 1, height);

      vector<int> arr = 
                {
                current[x1 + y1].getValue(),
                current[x1 + current[i].getColumn()].getValue(), 
                current[x1 + y2].getValue(),
                current[x3 + y1].getValue(),
                current[x3 + y2].getValue(),
                current[x2 + y1].getValue(),
                current[x2 + current[i].getColumn()].getValue(),
                current[x2 + y2].getValue(),
                };
      return arr;
    }
};