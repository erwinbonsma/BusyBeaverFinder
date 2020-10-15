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

    int metaLoopStart = idx1 + 1;
    if (metaLoopStart == _metaLoopStart) {
        // Nothing needs doing. We already analyzed this loop.
        return true;
    }

    // Ensure the loop starts so that it ends when a lower-level loop exited
    int startRunBlockIndex = endIndex - numMetaIterations * metaLoopPeriod + 1;
    int endRunBlockIndex = startRunBlockIndex + metaLoopPeriod - 1;

    _metaLoopStart = metaLoopStart;
    _loopStart = runSummary.runBlockAt(startRunBlockIndex)->getStartIndex();
    int loopEnd = runSummary.runBlockAt(endRunBlockIndex)->getStartIndex()
                  + runSummary.getRunBlockLength(endRunBlockIndex);
    int loopPeriod = loopEnd - _loopStart;

    return _loop.analyseLoop(_searcher.getInterpretedProgram(), runSummary, _loopStart, loopPeriod);
}

Trilian StaticMetaPeriodicHangDetector::proofHang() {
    int loopLen = _searcher.getRunSummary().getNumProgramBlocks() - _loopStart;

    if (loopLen % _loop.loopSize() != 0) {
        // The meta loop may contain multiple loops, which may trigger an invocation of proofHang
        // that is not in sync with the analyzed loop. Ignore these.
        return Trilian::MAYBE;
    }

    return StaticPeriodicHangDetector::proofHang();
}

void StaticMetaPeriodicHangDetector::reset() {
    StaticPeriodicHangDetector::reset();

    _metaLoopStart = -1;
}
