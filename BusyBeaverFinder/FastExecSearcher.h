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
    BaseSearchSettings _settings;
    FastExecutor _executor;
    std::string _programSpec;
    std::shared_ptr<InterpretedProgram> _interpretedProgram;

    int _totalRuns {};
public:
    FastExecSearcher(BaseSearchSettings settings);

    const std::string getProgramSpec() const override { return _programSpec; };
    int getNumSteps() const override { return _executor.numSteps(); };

    void run(const std::string& programSpec, std::shared_ptr<InterpretedProgram> program);

    void dumpSettings(std::ostream &os) const override;
    void dumpSearchProgress(std::ostream &os) const override;
};
