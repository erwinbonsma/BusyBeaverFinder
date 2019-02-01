//
//  Program.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <iostream>

#include "Program.h"

const char op_chars[5] = {'?', '_', 'o', '*', 'X' };

Program::Program(int width, int height) {
    _width = width;
    _height = height;

    // Initialize program
    //
    // All valid instructions are set to UNSET. All other positions are set to DONE. These can be
    // used to quickly check when the Program Pointer has exited the program thereby terminating
    // the program.
    //
    // Note, there's no extra rightmost column. This is not needed, the pointer will wrap and end
    // up in the leftmost "DONE" column.
    Op* pp = _ops;
    for (int row = -1; row <= MAX_HEIGHT; row++) {
        for (int col = -1; col < MAX_WIDTH; col++) {
            *pp = (row >=0 && row < height && col >= 0 && col < width) ? Op::UNSET : Op::DONE;
            pp++;
        }
    }
}

Op Program::getOp(int col, int row) {
    return _ops[(col + 1) + (row + 1) * (MAX_WIDTH + 1)];
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
