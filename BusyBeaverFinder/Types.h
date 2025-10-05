//
//  Types.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include <stdint.h>

// Program instructions
enum class Ins : int8_t {
    UNSET = 0,
    NOOP = 1,
    DATA = 2,
    TURN = 3,
    DONE = 4 // Guard instruction, signalling program completion
};

struct InstructionPointer {
    int8_t col;
    int8_t row;
};

// Program pointer directions
enum class Dir : int8_t {
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3
};

struct ProgramPointer {
    InstructionPointer p;
    Dir dir;
};

typedef int* DataPointer;

// Data operations
typedef uint8_t UndoOp;

// Direction of turn
enum class TurnDirection : int8_t {
    COUNTERCLOCKWISE = 0,
    CLOCKWISE = 1,
    NONE = -1
};

const int numDataDirections = 2;
enum class DataDirection : int8_t {
    LEFT = 0,
    RIGHT = 1,
};

enum class ValueChange : int8_t {
    // Value remains constant: x[n+1] = x[n]
    NONE = 0,

    // Data value increases/decreases linearly: x[n+1] = x[n] + C, with C != 0
    LINEAR = 1,

    // Value changes non-linearly
    OTHER = 2,
};

enum class LoopRunState : int8_t {
    // When execution is not in a loop (of when this is not yet detected).
    NO_LOOP,

    // When it has just been detected that the program entered a loop.
    STARTED,

    // When a loop is running and continuing execution.
    RUNNING,

    // When the loop just terminated.
    ENDED
};

// Describes how the loop behaves in the context of the program that it is part of
enum class LoopType : int8_t {
    // Stationary loop that traverses the same part of the data tape each time the loop runs.
    STATIONARY = 0,

    // Stationary loop that starts at a different location each time the loop runs. It "glides"
    // across the data tape. It typically does this by decreasing one counter, while increasing
    // another counter. It consumes a different counter each time the loop runs.
    GLIDER = 1,

    // Sweep loop with one fixed end
    ANCHORED_SWEEP = 2,

    // Sweep loop that extends its end points at both ends of the sweep. Note, it may take multiple
    // iterations of the sweep loop before it extends the sweep. This is for example the case for
    // irregular sweeps with binary-counting logic at the end of the sweep.
    DOUBLE_SWEEP = 3,
};

const int numHangTypes = 9;
const int numDetectedHangTypes = 8;
enum class HangType : int8_t {
    UNKNOWN = 0,
    // Simple loop, without any DATA
    NO_DATA_LOOP = 1,
    NO_EXIT = 2,
    PERIODIC = 3,
    META_PERIODIC = 4,
    REGULAR_SWEEP = 5,
    IRREGULAR_SWEEP = 6,
    APERIODIC_GLIDER = 7,
    UNDETECTED = 8 // Should always be last
};

enum class Trilian : int8_t {
    NO = 0,
    YES = 1,
    MAYBE = 2
};

enum class RunResult {
    UNKNOWN = 0,

    // Program completed successfully
    SUCCESS = 1,

    // Error execution data instruction
    DATA_ERROR = 2,

    // Encountered an unfinalized program block
    PROGRAM_ERROR = 3,

    // A hang detector detected a hang
    DETECTED_HANG = 4,

    // The program did not complete within the limit allowed steps
    ASSUMED_HANG = 5,
};
