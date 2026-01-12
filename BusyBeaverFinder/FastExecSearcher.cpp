//
//  FastExecSearcher.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 12/01/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//

#include "FastExecSearcher.h"

FastExecSearcher::FastExecSearcher(BaseSearchSettings settings)
    : Searcher(settings.size), _dataSize(settings.dataSize), _executor(settings.dataSize)
{
    _executor.setMaxSteps(settings.maxSteps);
}

void FastExecSearcher::run(std::string& programSpec) {
    _totalRuns++;

    // TODO
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
