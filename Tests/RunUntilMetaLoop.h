//
//  RunUntilMetaLoop.h
//  BusyBeaverFinder
//
//  Created by Erwin on 16/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include "HangDetector.h"

class RunUntilMetaLoop : public HangDetector {
    int _numIterations;

protected:
    bool shouldCheckNow() const override {
        return (_execution.getLoopRunState() == LoopRunState::ENDED
                && _execution.getMetaRunSummary().isInsideLoop()
                && _execution.getMetaRunSummary().getLoopIteration() >= _numIterations);
    }

    bool analyzeHangBehaviour() override { return true; };
    Trilian proofHang() override { return Trilian::YES; };

public:
    RunUntilMetaLoop(const ExecutionState& execution, int numIterations = 3)
    : HangDetector(execution), _numIterations(numIterations) {}
};

class RunUntilMetaMetaLoop : public RunUntilMetaLoop {
protected:
    bool shouldCheckNow() const override {
        return (_execution.getMetaMetaRunSummary().isInsideLoop()
                && RunUntilMetaLoop::shouldCheckNow());
    }

    bool analyzeHangBehaviour() override { return true; };
    Trilian proofHang() override { return Trilian::YES; };

public:
    RunUntilMetaMetaLoop(const ExecutionState& execution, int numIterations)
    : RunUntilMetaLoop(execution, numIterations) {}
};
