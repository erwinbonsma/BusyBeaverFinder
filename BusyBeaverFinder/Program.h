//
//  Program.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef Program_h
#define Program_h

#include <stdio.h>

#include "Consts.h"
#include "Enums.h"

typedef unsigned long long ulonglong;

const int programStorageSize = (MAX_WIDTH + 1) * (MAX_HEIGHT + 2);

class Program {
    int _width;
    int _height;

    // Instruction array
    Op _ops[programStorageSize];

public:
    Program(int width, int height);

    void clone(Program& dest);

    int getWidth() { return _width; }
    int getHeight() { return _height; }

    Op* startProgramPointer() { return &(_ops[1]); /* Start at row = -1, col = 0 */ }

    void setOp(Op *pp, Op op) { (*pp) = op; }
    void clearOp(Op *pp) { (*pp) = Op::UNSET; }
    Op getOp(Op *pp) { return (*pp); }
    Op getOp(int col, int row);

    /* Returns the number of possible programs in the search space that this program represents.
     * The summed total over all programs visited can be used to get an indication of the search
     * progress so far. However, it should be used with care. A very small number of programs
     * (those with zero or more NOOPs in the first column followed by a TURN) represent half of the
     * search space.
     *
     * Note: This functions returns 0 for certain programs. This is for programs whose last
     * instruction before termination was DATA. In this case, the same program with a NOOP is of
     * equivalent length. Not considering these two variations as separate programs means that
     * total number of programs is smaller. This way, the total number of 7x7 programs nearly fits
     * in an ulonglong representation.
     *
     * The total number of possible programs of size WxH = 3^((W - 1) * (H - 1)) * 2^(W + H - 2)
     */
    ulonglong getEquivalenceNumber();

    void dump();
};

#endif /* Program_h */
