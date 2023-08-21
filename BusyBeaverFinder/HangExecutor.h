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
#include "ProgramExecutor.h"
#include "RunSummary.h"
#include "Types.h"

class HangDetector;

struct ExecutionStackFrame {
    const ProgramBlock* programBlock;
    size_t dataStackSize;
    int numSteps;
};

class HangExecutor : public ProgramExecutor, public ExecutionState {
    std::vector<HangDetector*> _hangDetectors;
    std::vector<ExecutionStackFrame> _executionStack;

    int _hangDetectionStart;
    int _maxHangDetectionSteps;

    const InterpretedProgram *_program;

    Data _data;
    const HangDetector* _detectedHang;

    // Nested run summaries. The first summarizes the program execution, identifying loops along the
    // way. The second summarizes the first run summary. In particular, it signals repeated patterns
    // in the first summary.
    RunSummary _runSummary[2];
    int* _zArrayHelperBuf;

    RunResult executeBlock();

    RunResult executeWithoutHangDetection(int stepLimit);
    RunResult executeWithHangDetection(int stepLimit);

    RunResult run();

public:
    HangExecutor(int dataSize, int maxHangDetectionSteps);
    ~HangExecutor();

    // Only applies to the next invocation of execute, after which it is reset to zero.
    void setHangDetectionStart(int numSteps) { _hangDetectionStart = numSteps; }

    HangType detectedHangType() const override;
    const HangDetector* detectedHang() const { return _detectedHang; }

    void pop() override;

    RunResult execute(const InterpretedProgram* program) override;
    RunResult execute(std::string programSpec);

    void dump() const override;

    //----------------------------------------------------------------------------------------------
    // Implement ExecutionState interface

    const InterpretedProgram* getInterpretedProgram() const override { return _program; }

    const Data& getData() const override { return _data; }

    const RunSummary& getRunSummary() const override { return _runSummary[0]; }
    const RunSummary& getMetaRunSummary() const override { return _runSummary[1]; }

    void dumpExecutionState() const override;
};
