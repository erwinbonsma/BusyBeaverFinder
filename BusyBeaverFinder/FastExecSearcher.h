//
//  FastExecSearcher.h
//  BusyBeaverFinder
//
//  Created by Erwin on 12/01/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//
#pragma once

#include <memory>

#include "Searcher.h"
#include "FastExecutor.h"
#include "InterpretedProgramBuilder.h"

class FastExecSearcher : public Searcher {
    int _dataSize;
    FastExecutor _executor;
    std::shared_ptr<InterpretedProgramBuilder> _interpretedProgram;

    int _totalRuns {};
public:
    FastExecSearcher(BaseSearchSettings settings);

    int getNumSteps() const override { return _executor.numSteps(); };

    void run(std::string& programSpec);

    void dumpSettings(std::ostream &os) const override;
    void dumpSearchProgress(std::ostream &os) const override;
};
