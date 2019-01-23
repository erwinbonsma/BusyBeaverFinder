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

Program::Program(int width, int height) {
    _width = width;
    _height = height;
    _ops = new Op[width * height];

    for (int i = width * height; --i >= 0; ) {
        _ops[i] = Op::UNSET;
    }
}

void Program::clone(Program& dest) {
    for (int x = 0; x < _width; x++) {
        for (int y = 0; y < _height; y++) {
            dest.setOp(x, y, getOp(x, y));
        }
    }
}

void Program::dump() {
    for (int y = _height; --y >= 0; ) {
        for (int x = 0; x < _width; x++) {
            std::cout << " " << op_chars[(int)getOp(x, y)];
        }
        std::cout << std::endl;
    }
}
