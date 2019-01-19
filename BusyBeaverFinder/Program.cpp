//
//  Program.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#include <iostream>

#include "Program.h"

const char op_chars[4] = {'?', '_', 'o', '*' };

Program::Program() {
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            _ops[x][y] = Op::UNSET;
        }
    }
}

void Program::clone(Program& dest) {
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            dest.setOp(x, y, getOp(x, y));
        }
    }
}

void Program::dump() {
    for (int y = h; --y >= 0; ) {
        for (int x = 0; x < w; x++) {
            std::cout << " " << op_chars[(int)_ops[x][y]];
        }
        std::cout << "\n";
    }
}
