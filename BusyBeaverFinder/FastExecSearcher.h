//
//  FastExecSearcher.h
//  BusyBeaverFinder
//
//  Created by Erwin on 12/01/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//
#pragma once

#include "Searcher.h"
#include "FastExecutor.h"

class FastExecSearcher : public Searcher {
    int _dataSize;
    FastExecutor _executor;
    int _totalRuns {};
public:
    FastExecSearcher(BaseSearchSettings settings);

    int getNumSteps() const override { return _executor.numSteps(); };

    void run(std::string& programSpec);

    void dumpSettings(std::ostream &os) const override;
    void dumpSearchProgress(std::ostream &os) const override;
};
