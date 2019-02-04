//
//  Enums.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef Enums_h
#define Enums_h

#include "Consts.h"

enum class Op : char {
    UNSET = 0,
    NOOP = 1,
    DATA = 2,
    TURN = 3,
    DONE = 4 // Guard instruction, signalling program completion
};

enum class Dir : int {
    UP = (MAX_WIDTH + 1),
    RIGHT = 1,
    DOWN = -(MAX_WIDTH + 1),
    LEFT = -1
};

enum class DataOp : char {
    INC = 0,
    DEC = 1,
    SHR = 2,
    SHL = 3,
    NONE = 4
};

enum class SnapShotComparison : char {
    // Data did not change
    UNCHANGED = 0,
    // Data changed, but diverging from zero (which will not impact TURN evaluation)
    DIVERGING = 1,
    // Data changed in a way that can impact program flow (i.e. towards zero)
    IMPACTFUL = 2
};

#endif /* Enums_h */
