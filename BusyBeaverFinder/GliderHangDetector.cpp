//
//  GliderHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "GliderHangDetector.h"

#include <iostream>

#include "ExhaustiveSearcher.h"

GliderHangDetector::GliderHangDetector(ExhaustiveSearcher& searcher) :
    _searcher(searcher)
{
}

HangDetectionResult GliderHangDetector::start() {
    // Check basic assumption. The meta-run summary should be in a loop with period two:
    // one ever-increasing loop, one fixed switch
    RunSummary& metaRunSummary = _searcher.getMetaRunSummary();

    if ( !metaRunSummary.isInsideLoop() || metaRunSummary.getLoopPeriod() != 2 ) {
        return HangDetectionResult::FAILED;
    }

    _metaLoopIndex = metaRunSummary.getNumRunBlocks();

    _numLoopExits = 0;
    _numStepsAtLastLoopExit = 0;
    _dataPointerAtLoopStart = nullptr;
    _previousLoopLength = 0;
    _searcher.getDataTracker().reset();

    return HangDetectionResult::ONGOING;
}

HangDetectionResult GliderHangDetector::signalLoopStartDetected() {
    // Reset visited bounds so that the bounds of the various snapshots are comparable, irrespective
    // of the exact momemt when the hang check was started.
    _searcher.getData().resetVisitedBounds();

    return HangDetectionResult::ONGOING;
}

HangDetectionResult GliderHangDetector::signalLoopExit() {
//    _searcher.dumpHangDetection();

    if (_searcher.getMetaRunSummary().getNumRunBlocks() != _metaLoopIndex) {
        // We exited the assumed endless sweep meta-loop
        return HangDetectionResult::FAILED;
    }

    // Check that the glider loop is increasing in length. This, amongst others, prevents that
    // some periodic hangs are falsely reported as aperiodic glider hangs.
    if (_numStepsAtLastLoopExit > 0) {
        int loopLength = _searcher.getNumSteps() - _numStepsAtLastLoopExit;
        if (loopLength <= _previousLoopLength) {
            return HangDetectionResult::FAILED;
        }
        _previousLoopLength = loopLength;
    }
    _numStepsAtLastLoopExit = _searcher.getNumSteps();

    if (++_numLoopExits > 2) {
        if ( !_searcher.getDataTracker().gliderHangDetected() ) {
            return HangDetectionResult::FAILED;
        }

        if (_numLoopExits > 3) {
            // Conclude it's a hang after two successful checks (just in case)
            return HangDetectionResult::HANGING;
        }
    }

    _searcher.getDataTracker().captureSnapShot();
    _dataPointerAtLoopStart = nullptr;

    return HangDetectionResult::ONGOING;
}

HangDetectionResult GliderHangDetector::signalLoopIteration() {
    if (_dataPointerAtLoopStart == nullptr) {
        _dataPointerAtLoopStart = _searcher.getData().getDataPointer();
    }
    else if (_dataPointerAtLoopStart != _searcher.getData().getDataPointer()) {
        // Execution of the inner-loop of the glider should not result in effective DP-movements
        // (inside the loop, DP is allowed to temporarily move). Effective DP-movement should only
        // be done by the meta-loop, which causes the glider to move.
        return HangDetectionResult::FAILED;
    }

    return HangDetectionResult::ONGOING;
}
