//
//  Enums.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#ifndef Enums_h
#define Enums_h

enum class Op : char {
    UNSET = 0,
    NOOP = 1,
    DATA = 2,
    TURN = 3
};

enum class Dir : char {
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3
};

enum class DataOp : char {
    INC = 0,
    DEC = 1,
    SHR = 2,
    SHL = 3,
    NONE = 4
};

#endif /* Enums_h */
