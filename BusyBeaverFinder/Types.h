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
    int col;
    int row;
};

// Program pointer directions
enum class Dir : int {
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

const int numHangTypes = 8;
const int numDetectedHangTypes = 7;
enum class HangType : int8_t {
    // Simple loop, without any DATA
    NO_DATA_LOOP = 0,
    NO_EXIT = 1,
    PERIODIC = 2,
    META_PERIODIC = 3,
    REGULAR_SWEEP = 4,
    IRREGULAR_SWEEP = 5,
    APERIODIC_GLIDER = 6,
    UNDETECTED = 7 // Should always be last
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
