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
const uint8_t ins_vals[5] = {0, 0, 1, 2, 0};

const Ins ins_lookup[4] = { Ins::NOOP, Ins::DATA, Ins::TURN, Ins::UNSET };

// Mapping from '+' to 'z'
// Also supports filename + URL safe alternative coding, where '+' => '-' and '/' => '_'
const uint8_t b64_lookup[] = {
    62,  255, 62,  255, 63,  52,  53, 54, 55, 56, 57, 58, 59, 60, 61, 255,
    255, 0,   255, 255, 255, 255, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10,  11,  12,  13,  14,  15,  16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
    255, 255, 255, 255, 63,  255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36,  37,  38,  39,  40,  41,  42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

char charForVal(int v) {
    return (v == 0) ? '_' : 'a' + (v - 1);
}

std::vector<uint8_t> b64_decode(std::string in) {
    std::vector<uint8_t> out;
    out.reserve((in.size() * 6 + 7) / 8);

    int val = 0, valb = -8;
    for (uint8_t c : in) {
      if (c < '+' || c > 'z') break;
      c -= '+';
      if (b64_lookup[c] >= 64) break;
      val = (val << 6) + b64_lookup[c];
      valb += 6;
      if (valb >= 0) {
        out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
        valb -= 8;
      }
    }

    return out;
}

Program Program::fromString(std::string s) {
    auto bytes = b64_decode(s);
    uint8_t w = bytes[0] / 16;
    uint8_t h = bytes[0] % 16;

    InstructionPointer insP = { .col = 0, .row = static_cast<int8_t>(h - 1) };
    Program prog = Program(w, h);

    int p = 1, shift = 6;
    uint8_t byte = bytes[p];

    while (insP.row >= 0) {
        prog.setInstruction(insP, ins_lookup[(byte >> shift) % 4]);

        if (++insP.col == w) {
            --insP.row;
            insP.col = 0;
        }

        if (shift == 0) {
            shift = 6;
            byte = bytes[++p];
        } else {
            shift -= 2;
        }
    }

    return prog;
}

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

std::string Program::toString() const {
    std::string s;
    s.reserve((_height * _width + 2) / 3);
    int n = 0, v = 0;

    s += ('0' + _width);
    s += ('0' + _height);

    for (int y = _height; --y >= 0; ) {
        for (int x = 0; x < _width; x++) {
            v *= 3;
            v += ins_vals[(int)getInstruction(x, y)];
            ++n;
            if (n == 3) {
                s += charForVal(v);
                n = 0;
                v = 0;
            }
        }
    }

    if (n != 0) {
        while (n != 3) {
            v *= 3;
            ++n;
        }
        s += charForVal(v);
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
