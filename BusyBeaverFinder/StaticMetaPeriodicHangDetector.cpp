//
//  StaticMetaPeriodicHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 18/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticMetaPeriodicHangDetector.h"

StaticMetaPeriodicHangDetector::StaticMetaPeriodicHangDetector(ExhaustiveSearcher& searcher)
    : StaticPeriodicHangDetector(searcher) {}

Trilian StaticMetaPeriodicHangDetector::exhibitsHangBehaviour(bool loopContinues) {
    RunSummary& metaRunSummary = _searcher.getMetaRunSummary();
    RunSummary& runSummary = _searcher.getRunSummary();

    if (!metaRunSummary.isInsideLoop()) {
        return Trilian::NO;
    }

    if (!loopContinues) {
        // Too early to tell. Wait for the inner-loop to finish
        return Trilian::MAYBE;
    }

    int metaLoopPeriod = metaRunSummary.getLoopPeriod();
    _loopLength = 0;

    // Points to the last finalized run-block that is part of the meta-loop. The last run block in
    // the summary is skipped, as it is not yet finalized.
    int endIndex = runSummary.getNumRunBlocks() - 2;

    // It should be a loop, as this
    assert(runSummary.runBlockAt(endIndex)->isLoop());

    for (int i = 0; i < metaLoopPeriod; i++) {
        int idx1 = endIndex - i;
        int len1 = runSummary.getRunBlockLength(idx1);
        RunBlock* runBlock1 = runSummary.runBlockAt(idx1);

        _loopLength += len1;

        if (runBlock1->isLoop()) {
            int idx2 = idx1 - metaLoopPeriod;
            int len2 = runSummary.getRunBlockLength(idx2);

            if (runBlock1->getSequenceIndex() != runSummary.runBlockAt(idx2)->getSequenceIndex()) {
                // Can happen when the meta-run loop was just detected. In that case, idx2 may just
                // be outside the loop.
                return Trilian::MAYBE;
            }

            if (len1 != len2) {
                // All loops should be the same size
                // TODO: Check if this should this be MAYBE? Should we allow for a bootstrap period?
                return Trilian::NO;
            }
        }
    }

    return Trilian::YES;
}

bool StaticMetaPeriodicHangDetector::analyseLoop(LoopAnalysis &loop, int &loopStart) {
    // TODO

    return false;
}
