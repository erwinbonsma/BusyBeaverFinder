//
//  HangExecutor.h
//  BusyBeaverFinder
//
//  Created by Erwin on 12/08/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include <memory>
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
    std::vector<std::shared_ptr<HangDetector>> _hangDetectors;
    std::vector<ExecutionStackFrame> _executionStack;

    int _hangDetectionStart;
    int _maxHangDetectionSteps;

    const InterpretedProgram *_program;

    Data _data;
    std::shared_ptr<HangDetector> _detectedHang;

    // Program execution summaries at different levels. The run history is a log of executed
    // program blocks. The run summary summarizes this by identifying (and collapsing) loops. The
    // meta-summary summarizes the first run summary. In particular, it signals repeated patterns
    // in the first summary.
    RunHistory _runHistory;
    RunSummary _runSummary;
    MetaRunSummary _metaRunSummary;
    int* _zArrayHelperBuf;

    void resetHangDetection();

    RunResult executeBlock();

    RunResult executeWithoutHangDetection(int stepLimit);
    RunResult executeWithHangDetection(int stepLimit);

    RunResult run();

public:
    HangExecutor(int dataSize, int maxHangDetectionSteps);
    ~HangExecutor();

    void addDefaultHangDetectors();
    void addHangDetector(std::shared_ptr<HangDetector> hangDetector) {
        _hangDetectors.push_back(hangDetector);
    }

    // Only applies to the next invocation of execute, after which it is reset to zero.
    void setHangDetectionStart(int numSteps) { _hangDetectionStart = numSteps; }

    HangType detectedHangType() const override;
    std::shared_ptr<HangDetector> detectedHang() const { return _detectedHang; }

    void pop() override;

    RunResult execute(const InterpretedProgram* program) override;
    RunResult execute(std::string programSpec);

    void dump() const override;

    //----------------------------------------------------------------------------------------------
    // Implement ExecutionState interface

    const InterpretedProgram* getInterpretedProgram() const override { return _program; }

    const Data& getData() const override { return _data; }

    const RunHistory& getRunHistory() const override { return _runHistory; }
    const RunSummary& getRunSummary() const override { return _runSummary; }
    const MetaRunSummary& getMetaRunSummary() const override { return _metaRunSummary; }

    void dumpExecutionState() const override;
};
