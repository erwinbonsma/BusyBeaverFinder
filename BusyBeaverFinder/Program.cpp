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

#ifdef HANG_DETECTION2
    _prevVisited = _visited1;
    _activeVisited = _visited2;
    resetHangDetection();
#endif
}

void Program::resetHangDetection() {
#ifdef HANG_DETECTION2
    _firstPeriod = true;
#endif
}

bool Program::isHangDetected() {
#ifdef HANG_DETECTION2
    bool *p1 = _prevVisited;
    bool *p2 = _activeVisited;
    bool hangDetected = !_firstPeriod; // First run never detects hangs
    int i = programStorageSize;

    // Check and clear
    while (--i >= 0) {
        if (*p2 && !(*p1)) {
            hangDetected = false;
            break;
        }
        p2++;
        *(p1++) = false; // Clear previous for next period
    }

    // Clear remainder
    while (--i >= 0) {
        *(p1++) = false; // Clear previous for next period
    }

    // Swap arrays
    bool *tmp = _prevVisited;
    _prevVisited = _activeVisited;
    _activeVisited = tmp;

    _firstPeriod = false;

    return hangDetected;
#else
    return false;
#endif
}

Op Program::getOp(int col, int row) {
    return _ops[(col + 1) + (row + 1) * (MAX_WIDTH + 1)];
}


void Program::clone(Program& dest) {
    Op* ppSrc = startProgramPointer();
    Op* ppDst = dest.startProgramPointer();
    for (int i = programStorageSize; --i >= 0; ) {
        dest.setOp(ppDst++, getOp(ppSrc++));
    }
}

ulonglong Program::getEquivalenceNumber() {
    ulonglong num = 1;
    Op op;
    for (int x = _width - 1; --x >= 0; ) {
        for (int y = _height - 1; --y >= 0; ) {
            if (getOp(x, y) == Op::UNSET) {
                num *= 3;
            }
        }

        op = getOp(x, _height - 1);
        if (op == Op::UNSET) {
            num *= 2;
        }
        else if (op == Op::DATA) {
            return 0;
        }
    }
    for (int y = _height - 1; --y >= 0; ) {
        op = getOp(_width - 1, y);
        if (op == Op::UNSET) {
            num *= 2;
        }
        else if (op == Op::DATA) {
            return 0;
        }
    }
    return num;
}

void Program::dump() {
    for (int y = _height; --y >= 0; ) {
        for (int x = 0; x < _width; x++) {
            std::cout << " " << op_chars[(int)getOp(x, y)];
        }
        std::cout << std::endl;
    }
}

void Program::dumpHangInfo() {
#ifdef HANG_DETECTION2
    bool *p1 = _prevVisited;
    bool *p2 = _activeVisited;
    for (int i = programStorageSize; --i >= 0; ) {
        std::cout << *(p1++) << "/" << *(p2++) << " ";
    }
    std::cout << std::endl;
#endif
}
