//
//  Program.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#ifndef Program_h
#define Program_h

#include <stdio.h>

#include "Consts.h"
#include "Enums.h"

class Program {
    int _width;
    int _height;
    // Instruction array
    Op *_ops;

public:
    Program(int width, int height);

    void clone(Program& dest);

    int getWidth() { return _width; }
    int getHeight() { return _height; }

    void setOp(int x, int y, Op op) { _ops[x + y * _width] = op; }
    void clearOp(int x, int y) { _ops[x + y * _width] = Op::UNSET; }
    Op getOp(int x, int y) { return _ops[x + y * _width]; }

    void dump();
};

#endif /* Program_h */
