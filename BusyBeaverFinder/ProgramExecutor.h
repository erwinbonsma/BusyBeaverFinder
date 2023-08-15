//
//  ProgramExecutor.h
//  BusyBeaverFinder
//
//  Created by Erwin on 13/08/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include "Types.h"

class InterpretedProgram;
class ProgramBlock;

class ProgramExecutor {

protected:
    int _maxSteps;
    int _numSteps;

    const ProgramBlock* _block;

public:
    void setMaxSteps(int steps) { _maxSteps = steps; }
    int numSteps() const { return _numSteps; }

    const ProgramBlock* lastProgramBlock() { return _block; }
    virtual HangType detectedHangType() const = 0;

    virtual RunResult execute(const InterpretedProgram* program) = 0;
    virtual RunResult resume() = 0;
    virtual void dump() const = 0;
};
