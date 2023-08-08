//
//  Program.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <iostream>

#include "Program.h"

const char ins_chars[5] = {'?', '_', 'o', '*', 'X' };
const char web_chars[5] = {'_', '_', 'o', '*', 'X' };

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
    _instructions.clear();
    for (int row = -1; row <= maxHeight; row++) {
        for (int col = -1; col < maxWidth; col++) {
            _instructions.push_back((row >=0 && row < height && col >= 0 && col < width)
                                    ? Ins::UNSET : Ins::DONE);
        }
    }
}

int Program::indexFor(InstructionPointer insP) const {
    return (insP.col + 1) + (insP.row + 1) * (maxWidth + 1);
}

Ins Program::getInstruction(int col, int row) const {
    return _instructions[(col + 1) + (row + 1) * (maxWidth + 1)];
}

void Program::clone(Program& dest) const {
    dest._height = _height;
    dest._width = _width;
    dest._instructions = _instructions;
}

ulonglong Program::getEquivalenceNumber() {
    ulonglong num = 1;
    Ins ins;
    for (int x = _width - 1; --x >= 0; ) {
        for (int y = _height - 1; --y >= 0; ) {
            if (getInstruction(x, y) == Ins::UNSET) {
                num *= 3;
            }
        }

        ins = getInstruction(x, _height - 1);
        if (ins == Ins::UNSET) {
            num *= 2;
        }
        else if (ins == Ins::DATA) {
            return 0;
        }
    }
    for (int y = _height - 1; --y >= 0; ) {
        ins = getInstruction(_width - 1, y);
        if (ins == Ins::UNSET) {
            num *= 2;
        }
        else if (ins == Ins::DATA) {
            return 0;
        }
    }
    return num;
}

std::string Program::toWebString() const {
    std::string s;
    s.reserve(_height * _width);

    for (int y = _height; --y >= 0; ) {
        for (int x = 0; x < _width; x++) {
            s += web_chars[(int)getInstruction(x, y)];
        }
    }

    return s;
}

void Program::dump() const {
    for (int y = _height; --y >= 0; ) {
        for (int x = 0; x < _width; x++) {
            std::cout << " " << ins_chars[(int)getInstruction(x, y)];
        }
        std::cout << std::endl;
    }
}

void Program::dumpWeb() const {
    std::cout << toWebString() << std::endl;
}

void Program::dump(InstructionPointer insP) const {
    char sepChar = ' ';
    for (int row = _height; --row >= 0; ) {
        for (int col = 0; col < _width; col++) {
            bool isActiveInstruction = (insP.col == col) && (insP.row == row);
            if (isActiveInstruction) {
                sepChar = '[';
            }
            std::cout << sepChar << ins_chars[(int)getInstruction(col, row)];
            sepChar = (isActiveInstruction) ? ']' : ' ';
        }
        std::cout << sepChar << std::endl;
    }
}
