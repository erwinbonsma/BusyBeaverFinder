//
//  HangExecutor.h
//  BusyBeaverFinder
//
//  Created by Erwin on 12/08/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include <vector>

#include "Data.h"
#include "ExecutionState.h"
#include "RunSummary.h"
#include "Types.h"

class HangDetector;

class HangExecutor : public ExecutionState {
    std::vector<HangDetector*> _hangDetectors;

    int _maxSteps;
    int _hangDetectionEnd;

    const InterpretedProgram *_program;

    Data _data;
    const ProgramBlock* _block;
    HangType _detectedHangType;
    int _numSteps;

    // Nested run summaries. The first summarizes the program execution, identifying loops along the
    // way. The second summarizes the first run summary. In particular, it signals repeated patterns
    // in the first summary.
    RunSummary _runSummary[2];
    int* _zArrayHelperBuf;


    RunResult executeBlock();
    RunResult executeWithoutHangDetection();

public:
    HangExecutor(int dataSize, int maxHangDetectionSteps);
    ~HangExecutor();

    void setMaxSteps(int steps) { _maxSteps = steps; }
    int numSteps() const { return _numSteps; }
    const ProgramBlock* lastProgramBlock() { return _block; }
    HangType detectedHangType() { return _detectedHangType; }

    RunResult execute(const InterpretedProgram* program);

    //----------------------------------------------------------------------------------------------
    // Implement ExecutionState interface

    const InterpretedProgram* getInterpretedProgram() const override { return _program; }

    const Data& getData() const override { return _data; }

    const RunSummary& getRunSummary() const override { return _runSummary[0]; }
    const RunSummary& getMetaRunSummary() const override { return _runSummary[1]; }

    void dumpExecutionState() const override;
};
