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

typedef Ins* InstructionPointer;

// Program pointer directions
enum class Dir : int {
    UP = (MAX_WIDTH + 1),
    RIGHT = 1,
    DOWN = -(MAX_WIDTH + 1),
    LEFT = -1
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

const int numHangTypes = 6;
const int numDetectedHangTypes = 5;
enum class HangType : char {
    NO_EXIT = 0,
    PERIODIC = 1,
    REGULAR_SWEEP = 2,
    IRREGULAR_SWEEP = 3,
    APERIODIC_GLIDER = 4,
    UNDETECTED = 5 // Should always be last
};

#endif /* Types_h */
