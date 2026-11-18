// A two-dimensional array that holds a pipeline table of instructions by cycles, with row sums, column sums and a transpose.

#include <iostream>
#include <string>

const int INSTRUCTIONS = 4;
const int CYCLES = 8;

// fills the grid for a pipeline without any stalls: instruction r enters the
// pipeline at cycle r, and spends one cycle in each of the five stages
void buildPipelineGrid(int grid[INSTRUCTIONS][CYCLES]) {
    for (int row = 0; row < INSTRUCTIONS; ++row) {
        for (int column = 0; column < CYCLES; ++column) {
            grid[row][column] = -1;                       // -1 means "not in the pipeline"
        }
    }
    for (int row = 0; row < INSTRUCTIONS; ++row) {
        for (int stage = 0; stage < 5; ++stage) {
            grid[row][row + stage] = stage;               // cell [instruction][cycle] = stage
        }
    }
}

// draws the grid as text
// how many instructions are in the pipeline during one cycle (a column sum)
int busyStagesInCycle(const int grid[INSTRUCTIONS][CYCLES], int column) {
    int busy = 0;
    for (int row = 0; row < INSTRUCTIONS; ++row) {
        if (grid[row][column] >= 0) {
            busy = busy + 1;
        }
    }
    return busy;
}

// how many cycles one instruction spends in the pipeline (a row sum)
int cyclesOfInstruction(const int grid[INSTRUCTIONS][CYCLES], int row) {
    int cycles = 0;
    for (int column = 0; column < CYCLES; ++column) {
        if (grid[row][column] >= 0) {
            cycles = cycles + 1;
        }
    }
    return cycles;
}

// swaps rows and columns: result[column][row] = grid[row][column]
void transpose(const int grid[INSTRUCTIONS][CYCLES], int result[CYCLES][INSTRUCTIONS]) {
    for (int row = 0; row < INSTRUCTIONS; ++row) {
        for (int column = 0; column < CYCLES; ++column) {
            result[column][row] = grid[row][column];
        }
    }
}
