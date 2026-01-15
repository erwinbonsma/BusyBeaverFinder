//
//  FastExecSearcher.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 12/01/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//

#include "FastExecSearcher.h"

FastExecSearcher::FastExecSearcher(BaseSearchSettings settings) :
    Searcher(settings.size),
    _dataSize(settings.dataSize),
    _executor(settings.dataSize),
    _interpretedProgram(std::make_shared<InterpretedProgramBuilder>())
{
    _executor.setMaxSteps(settings.maxSteps);
}

void FastExecSearcher::run(std::string& programSpec) {
    _totalRuns++;

    // TODO: Avoid re-allocating program
    _program = Program::fromString(programSpec);

    _interpretedProgram->buildFromProgram(_program);

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
    << "Size = " << _program.getSize()
    << ", DataSize = " << _dataSize
    << ", MaxSteps = " << _executor.getMaxSteps()
    << std::endl;
}

void FastExecSearcher::dumpSearchProgress(std::ostream &os) const {
    os
    << "#Runs = " << _totalRuns;
}
