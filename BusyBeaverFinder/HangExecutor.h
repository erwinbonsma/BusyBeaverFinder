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
#include "RunSummary.h"
#include "Types.h"

class HangDetector;

class HangExecutor {
    std::vector<HangDetector*> _hangDetectors;

    int _numSteps;
    int _maxSteps;
    int _hangDetectionEnd;

    // Nested run summaries. The first summarizes the program execution, identifying loops along the
    // way. The second summarizes the first run summary. In particular, it signals repeated patterns
    // in the first summary.
    RunSummary _runSummary[2];
    Data _data;
    const ProgramBlock* _block;

    RunResult executeBlock();
    RunResult executeWithoutHangDetection();

public:
    HangExecutor();
    ~HangExecutor();

    void setMaxSteps(int steps) { _maxSteps = steps; }
    int numSteps() const { return _numSteps; }

    RunResult execute(const ProgramBlock *block);
};
