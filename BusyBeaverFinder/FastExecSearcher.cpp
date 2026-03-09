//
//  FastExecSearcher.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 12/01/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//

#include "FastExecSearcher.h"

FastExecSearcher::FastExecSearcher(BaseSearchSettings settings) :
    _settings(settings),
    _executor(settings.dataSize)
{
    _executor.setMaxSteps(settings.maxSteps);
}

void FastExecSearcher::run(const std::string& programSpec,
                           std::shared_ptr<InterpretedProgram> program) {
    _totalRuns++;

    _programSpec = programSpec;
    _interpretedProgram = program;

    RunResult result = _executor.execute(_interpretedProgram);
    _executor.pop();

    switch (result) {
        case RunResult::DATA_ERROR:
            _tracker->reportError();
            return;
        case RunResult::SUCCESS:
            _tracker->reportDone(_executor.numSteps());
            return;
        case RunResult::ASSUMED_HANG:
            _tracker->reportAssumedHang();
            return;
        case RunResult::PROGRAM_ERROR:
            _tracker->reportLateEscape(_executor.numSteps());
            return;
        default:
            // Unexpected result
            assert(false);
    }
}

void FastExecSearcher::dumpSettings(std::ostream &os) const {
    os
    << "Size = " << _settings.size
    << ", DataSize = " << _settings.dataSize
    << ", MaxSteps = " << _settings.maxSteps
    << std::endl;
}

void FastExecSearcher::dumpSearchProgress(std::ostream &os) const {
    os
    << "#Runs = " << _totalRuns;
}
