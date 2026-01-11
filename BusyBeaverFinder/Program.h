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

struct ProgramSize {
    int8_t width = 0;
    int8_t height = 0;

    ProgramSize() {}
    explicit ProgramSize(int8_t size) : width(size), height(size) {}
    ProgramSize(int8_t w, int8_t h) : width(w), height(h) {}
};

class Program {
    ProgramSize _size;

    // Instruction array
    std::vector<Ins> _instructions;

    InstructionPointer getInstructionP(int col, int row) const {
        return InstructionPointer { .col = (int8_t)col, .row = (int8_t)row };
    }
    Ins getInstruction(int col, int row) const {
        return _instructions[(col + 1) + (row + 1) * (_size.width + 1)];
    }

    std::string toSimpleString(const char* charEncoding, bool addLineBreaks = false) const;
public:
    static Program fromString(std::string s);

    Program() {}
    explicit Program(ProgramSize size);

    void clone(Program& dest) const;

    ProgramSize getSize() const { return _size; }

    const Ins* getInstructionBuffer() const { return &_instructions[0]; }

    ProgramPointer getStartProgramPointer() {
        return ProgramPointer { .p = { .col = 0, .row = -1 }, .dir = Dir::UP };
    }

    int indexFor(InstructionPointer insP) const {
        return (insP.col + 1) + (insP.row + 1) * (_size.width + 1);
    }

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

std::ostream &operator<<(std::ostream &os, const ProgramSize &size);
