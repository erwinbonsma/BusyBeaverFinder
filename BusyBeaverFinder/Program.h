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
    Op _ops[w][h];

public:
    Program();

    void clone(Program& dest);

    void setOp(int x, int y, Op op) { _ops[x][y] = op; }
    void clearOp(int x, int y) { _ops[x][y] = Op::UNSET; }
    Op getOp(int x, int y) { return _ops[x][y]; }

    void dump();
};

#endif /* Program_h */
