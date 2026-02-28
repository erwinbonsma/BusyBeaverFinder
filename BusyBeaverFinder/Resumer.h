//
//  Resumer.h
//  BusyBeaverFinder
//
//  Created by Erwin on 28/02/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//
#pragma once

#include <vector>

#include "Types.h"
#include "Program.h"

class Resumer {
public:
    virtual ~Resumer() {}

    // Returns the next instruction from the search stack so that search continues from resume
    // point.
    virtual Ins popNextInstruction(InstructionPointer ip) = 0;

    // Returns true when resume stack is empty
    virtual bool isDone() = 0;
};

class ResumeFromStack : public Resumer {
    std::vector<Ins>::const_iterator _resumeIns;
    std::vector<Ins>::const_iterator _resumeEnd;
public:
    ResumeFromStack(const std::vector<Ins> &resumeFrom) :
    _resumeIns(resumeFrom.cbegin()), _resumeEnd(resumeFrom.cend()) {}

    Ins popNextInstruction(InstructionPointer ip) override { return *_resumeIns++; }
    bool isDone() override { return _resumeIns == _resumeEnd; }
};

class ResumeFromProgram : public Resumer {
    Program _program;
    // The number of set instructions yet to pop
    int _popCount;
public:
    ResumeFromProgram(const std::string& programSpec);

    Ins popNextInstruction(InstructionPointer ip) override;
    bool isDone() override { return _popCount == 0; }
};
