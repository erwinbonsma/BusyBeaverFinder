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

    // Notify the executor that the program has been shrunk (as a result of the search
    // back-tracking). Each invocation pairs up with an invocation of execute. This allows
    // executors that maintain a stack of program execution states to resume execution from the
    // previous state.
    virtual void pop() {};

    virtual RunResult execute(const InterpretedProgram* program) = 0;
    virtual void dump() const = 0;
};
