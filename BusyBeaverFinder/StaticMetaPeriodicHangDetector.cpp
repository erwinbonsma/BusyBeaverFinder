//
//  StaticMetaPeriodicHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 18/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticMetaPeriodicHangDetector.h"

#include <iostream>

StaticMetaPeriodicHangDetector::StaticMetaPeriodicHangDetector(ExhaustiveSearcher& searcher)
    : StaticPeriodicHangDetector(searcher) {}

bool StaticMetaPeriodicHangDetector::shouldCheckNow(bool loopContinues) {
    // Should wait for the inner-loop to finish
    return !loopContinues && _searcher.getMetaRunSummary().isInsideLoop();
}

bool StaticMetaPeriodicHangDetector::analyzeHangBehaviour() {
    RunSummary& runSummary = _searcher.getRunSummary();

    // Points to the last run-block that is part of the meta-loop. Although it is not yet finalized,
    // as it is a loop which will not continue, we know it will.
    int endIndex = runSummary.getNumRunBlocks() - 1;
    int metaLoopPeriod = _searcher.getMetaRunSummary().getLoopPeriod();

    // Determine how many of the last iterations of the meta-loop were exactly the same (i.e. all
    // inner-loops ran for the same duration).
    int idx1 = endIndex;
    int idx2 = idx1 - metaLoopPeriod;
    while (
        idx2 >= 0 &&
        runSummary.runBlockAt(idx1)->getSequenceIndex() ==
        runSummary.runBlockAt(idx2)->getSequenceIndex() &&
        runSummary.getRunBlockLength(idx1) == runSummary.getRunBlockLength(idx2)
    ) {
        idx1--;
        idx2--;
    };

    int numMetaIterations = (endIndex - idx2) / metaLoopPeriod;
    if (numMetaIterations <= 1) {
        return false;
    }

    int startRunBlockIndex = endIndex - numMetaIterations * metaLoopPeriod + 1;
    int endRunBlockIndex = startRunBlockIndex + metaLoopPeriod - 1;

    _loopStart = runSummary.runBlockAt(startRunBlockIndex)->getStartIndex();
    int loopEnd = runSummary.runBlockAt(endRunBlockIndex)->getStartIndex()
                  + runSummary.getRunBlockLength(endRunBlockIndex);
    int loopPeriod = loopEnd - _loopStart;

    return _loop.analyseLoop(_searcher.getInterpretedProgram(), runSummary, _loopStart, loopPeriod);
}
