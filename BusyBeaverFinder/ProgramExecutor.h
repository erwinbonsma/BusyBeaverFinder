//
//  ProgramExecutor.h
//  BusyBeaverFinder
//
//  Created by Erwin on 13/08/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include <memory>

#include "Types.h"

class InterpretedProgram;
class ProgramBlock;

class ProgramExecutor {

protected:
    size_t _maxSteps;
    size_t _numSteps;

    const ProgramBlock* _block;

public:
    virtual ~ProgramExecutor() {}

    void setMaxSteps(size_t steps) { _maxSteps = steps; }
    size_t getMaxSteps() const { return _maxSteps; }
    size_t numSteps() const { return _numSteps; }

    const ProgramBlock* lastProgramBlock() { return _block; }
    virtual HangType detectedHangType() const = 0;

    // Notify the executor that the program has been shrunk (as a result of the search
    // back-tracking). Each invocation pairs up with an invocation of execute. This allows
    // executors that maintain a stack of program execution states to resume execution from the
    // previous state.
    virtual void pop() {};

    virtual RunResult execute(std::shared_ptr<const InterpretedProgram> program) = 0;
    virtual void dump() const = 0;
};
