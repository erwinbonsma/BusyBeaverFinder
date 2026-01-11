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

    std::string toPlainString() const;
    std::string toWebString() const;
    std::string toString() const;

    void dump() const;
    void dumpWeb() const;
    void dump(InstructionPointer insP) const;
};
