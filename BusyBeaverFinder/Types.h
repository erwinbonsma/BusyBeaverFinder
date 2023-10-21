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

enum class BoundaryChange : int8_t {
    // Boundary is fixed
    FIXED = 0,

    // Boundary moves at fixed speed in direction that expands the region
    LINEAR_GROWTH = 1,

    // Boundary moves at fixed speed in direction that shrinks the region
    LINEAR_REDUCTION = 2,

    // Not supporting OTHER, as detection does not (yet) support this so can immediately abort and
    // does not need to store this.
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
