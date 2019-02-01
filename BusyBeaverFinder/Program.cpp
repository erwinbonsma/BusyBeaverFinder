//
//  Program.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <iostream>

#include "Program.h"

const char op_chars[4] = {'?', '_', 'o', '*' };

Program::Program(int width, int height) {
    _width = width;
    _height = height;

    Op* pp = _ops;
    for (int row = -1; row <= MAX_HEIGHT; row++) {
        for (int col = 0; col <= MAX_WIDTH; col++) {
            *pp = (row >=0 && row < height && col >= 0 && col < width) ? Op::UNSET : Op::DONE;
            pp++;
        }
    }
}

Op Program::getOp(int col, int row) {
    return _ops[col + (row + 1) * (MAX_WIDTH + 1)];
}


void Program::clone(Program& dest) {
    Op* ppSrc = startPP();
    Op* ppDst = dest.startPP();
    for (int i = programStorageSize; --i >= 0; ) {
        dest.setOp(ppDst++, getOp(ppSrc++));
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
