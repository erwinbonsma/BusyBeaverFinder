//
//  MetaPeriodicHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 18/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "MetaPeriodicHangDetector.h"

#include <limits>

#include "ExhaustiveSearcher.h"

MetaPeriodicHangDetector::MetaPeriodicHangDetector(ExhaustiveSearcher& searcher) :
    PeriodicHangDetector(searcher)
{
}

RunSummary* MetaPeriodicHangDetector::getTargetRunSummary() {
    return &_searcher.getMetaRunSummary();
}

bool MetaPeriodicHangDetector::isPeriodicLoopPattern() {
    RunSummary& metaRunSummary = _searcher.getMetaRunSummary();
    RunSummary& runSummary = _searcher.getRunSummary();

    if (!metaRunSummary.isInsideLoop()) {
        return false;
    }

    int metaLoopPeriod = metaRunSummary.getLoopPeriod();
    _loopLength = 0;

    // Skip the last, yet unfinalized, run block
    int startIndex = runSummary.getNumRunBlocks() - 2;
    for (int i = 0; i < metaLoopPeriod; i++) {
        int idx1 = startIndex - i;
        int len1 = runSummary.getRunBlockLength(idx1);
        RunBlock* runBlock1 = runSummary.runBlockAt(idx1);

        _loopLength += len1;

        if (runBlock1->isLoop()) {
            int idx2 = idx1 - metaLoopPeriod;
            int len2 = runSummary.getRunBlockLength(idx2);

            if (runBlock1->getSequenceIndex() != runSummary.runBlockAt(idx2)->getSequenceIndex()) {
                // Can happen when the meta-run loop was just detected. In that case, idx2 may just
                // be outside the loop.
                return false;
            }

            if (len1 != len2) {
                // All loops should be the same size
                return false;
            }
        }
    }

    return true;
}

void MetaPeriodicHangDetector::start() {
    PeriodicHangDetector::start();

    _abortTime = _searcher.getRunSummary().getNumProgramBlocks() + _loopLength;
}

void MetaPeriodicHangDetector::signalLoopIterationCompleted() {
    if (_searcher.getRunSummary().getNumProgramBlocks() > _abortTime) {
        // We may be stuck in an inner-loop instead of the assumed meta-loop.
        setHangDetectionResult(HangDetectionResult::FAILED);
    }
}

void MetaPeriodicHangDetector::signalLoopExit() {
    if (_status != HangDetectionResult::ONGOING) {
        return;
    }

    if (
        !_trackedRunSummary->isInsideLoop() ||
        _loopRunBlockIndex != _trackedRunSummary->getNumRunBlocks()
    ) {
        // Apparently not same periodic loop anymore
        setHangDetectionResult(HangDetectionResult::FAILED);
        return;
    }

    if (
        _searcher.getDataTracker().getNewSnapShot() == nullptr ||
        _searcher.getRunSummary().getNumProgramBlocks() == _timeOfNextSnapshot
    ) {
        captureAndCheckSnapshot();
        _timeOfNextSnapshot = _searcher.getRunSummary().getNumProgramBlocks() + _loopLength;
        _abortTime = _timeOfNextSnapshot;
    }
}
