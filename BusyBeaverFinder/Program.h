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

const int programStorageSize = (MAX_WIDTH + 1) * (MAX_HEIGHT + 2);

class Program {
    int _width;
    int _height;

    // Instruction array
    Op _ops[programStorageSize];

#ifdef HANG_DETECTION2
    // Hang detection
    bool* _activeVisited;
    bool* _prevVisited;
    bool _visited1[programStorageSize];
    bool _visited2[programStorageSize];
    bool _firstPeriod;
#endif

public:
    Program(int width, int height);

    void clone(Program& dest);

    int getWidth() { return _width; }
    int getHeight() { return _height; }

    Op* startPP() { return &(_ops[1]); /* Start at row = -1, col = 0 */ }

    void resetHangDetection();
    bool isHangDetected();

    void setOp(Op *pp, Op op) { (*pp) = op; }
    void clearOp(Op *pp) { (*pp) = Op::UNSET; }
    Op getOp(Op *pp) {
#ifdef HANG_DETECTION2
        _activeVisited[pp - _ops] = true;
#endif
        return (*pp);
    }
    Op getOp(int col, int row);

    void dump();
    void dumpHangInfo();
};

#endif /* Program_h */
