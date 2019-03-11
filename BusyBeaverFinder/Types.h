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

// Program pointer directions
enum class Dir : int {
    UP = (MAX_WIDTH + 1),
    RIGHT = 1,
    DOWN = -(MAX_WIDTH + 1),
    LEFT = -1
};

struct ProgramPointer {
    Ins* p;
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

const int numHangTypes = 5;
const int numDetectedHangTypes = 4;
enum class HangType : char {
    PERIODIC = 0,
    REGULAR_SWEEP = 1,
    IRREGULAR_SWEEP = 2,
    APERIODIC_GLIDER = 3,
    UNDETECTED = 4 // Should always be last
};

#endif /* Types_h */
