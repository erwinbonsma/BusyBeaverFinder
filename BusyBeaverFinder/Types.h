//
//  Types.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef Types_h
#define Types_h

#include "Consts.h"

// Program instructions
enum class Ins : char {
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
enum class DataOp : char {
    INC = 0,
    DEC = 1,
    SHR = 2,
    SHL = 3,
    NONE = 4
};

// Direction of turn
enum class TurnDirection : char {
    COUNTERCLOCKWISE = 0,
    CLOCKWISE = 1,
    NONE = -1
};

const int numHangTypes = 7;
const int numDetectedHangTypes = 6;
enum class HangType : char {
    // Simple loop, without any DATA
    NO_DATA_LOOP = 0,
    NO_EXIT = 1,
    PERIODIC = 2,
    REGULAR_SWEEP = 3,
    IRREGULAR_SWEEP = 4,
    APERIODIC_GLIDER = 5,
    UNDETECTED = 6 // Should always be last
};

enum class Trilian : char {
    NO = 0,
    YES = 1,
    MAYBE = 2
};

#endif /* Types_h */
