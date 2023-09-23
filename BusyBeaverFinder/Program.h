//
//  Program.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include <string>
#include <vector>

#include "Consts.h"
#include "Types.h"

typedef unsigned long long ulonglong;


class Program {
    int _width;
    int _height;

    // Instruction array
    std::vector<Ins> _instructions;

    InstructionPointer getInstructionP(int col, int row) const {
        return InstructionPointer { .col = (int8_t)col, .row = (int8_t)row };
    }
    Ins getInstruction(int col, int row) const;

    std::string toSimpleString(const char* charEncoding, bool addLineBreaks = false) const;
public:
    static Program fromString(std::string s);

    Program(int width, int height);

    void clone(Program& dest) const;

    int getWidth() const { return _width; }
    int getHeight() const { return _height; }

    const Ins* getInstructionBuffer() const { return &_instructions[0]; }

    ProgramPointer getStartProgramPointer() {
        return ProgramPointer { .p = { .col = 0, .row = -1 }, .dir = Dir::UP };
    }

    int indexFor(InstructionPointer insP) const;

    void setInstruction(InstructionPointer insP, Ins ins) { _instructions[indexFor(insP)] = ins; }
    void clearInstruction(InstructionPointer insP) { _instructions[indexFor(insP)] = Ins::UNSET; }
    Ins getInstruction(InstructionPointer insP) const { return _instructions[indexFor(insP)]; }

    /* Returns the number of possible programs in the search space that this program represents.
     * The summed total over all programs visited can be used to get an indication of the search
     * progress so far. However, it should be used with care. A very small number of programs
     * (those with zero or more NOOPs in the first column followed by a TURN) represent half of the
     * search space.
     *
     * Note: This functions returns 0 for certain programs. This is for programs whose last
     * instruction before termination was DATA. In this case, the same program with a NOOP is of
     * equivalent length. Not considering these two variations as separate programs means that
     * total number of programs is smaller. This way, the total number of 7x7 programs nearly fits
     * in an ulonglong representation.
     *
     * The total number of possible programs of size WxH = 3^((W - 1) * (H - 1)) * 2^(W + H - 2)
     */
    ulonglong getEquivalenceNumber();

    std::string toPlainString() const;
    std::string toWebString() const;
    std::string toString() const;

    void dump() const;
    void dumpWeb() const;
    void dump(InstructionPointer insP) const;
};
