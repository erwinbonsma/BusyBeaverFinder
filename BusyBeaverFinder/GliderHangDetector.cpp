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

void GliderHangDetector::start() {
    // Check basic assumption. The meta-run summary should be in a loop with period two:
    // one ever-increasing loop, one fixed switch
    RunSummary& metaRunSummary = _searcher.getMetaRunSummary();

    if ( !metaRunSummary.isInsideLoop() || metaRunSummary.getLoopPeriod() != 2 ) {
        _status = HangDetectionResult::FAILED;
    } else {
        _status = HangDetectionResult::ONGOING;
        _metaLoopIndex = metaRunSummary.getNumRunBlocks();
    }

    _numLoopExits = 0;
    _numStepsAtLastLoopExit = 0;
    _dataPointerAtLoopStart = nullptr;
    _previousLoopLength = 0;
    _searcher.getDataTracker().reset();
}

void GliderHangDetector::signalLoopStartDetected() {
    // Reset visited bounds so that the bounds of the various snapshots are comparable, irrespective
    // of the exact momemt when the hang check was started.
    _searcher.getData().resetVisitedBounds();
}

void GliderHangDetector::checkGliderContract() {
    if (!_searcher.getDataTracker().gliderHangDetected()) {
        _status = HangDetectionResult::FAILED;
    } else {
        // Possibly require multiple positive checks before concluding it is a hang
    }
}

void GliderHangDetector::signalLoopExit() {
//    _searcher.dumpHangDetection();

    if (_searcher.getMetaRunSummary().getNumRunBlocks() != _metaLoopIndex) {
        // We exited the assumed endless sweep meta-loop
        _status = HangDetectionResult::FAILED;
        return;
    }

    // Check that the glider loop is increasing in length. This, amongst others, prevents that
    // some periodic hangs are falsely reported as aperiodic glider hangs.
    if (_numStepsAtLastLoopExit > 0) {
        int loopLength = _searcher.getNumSteps() - _numStepsAtLastLoopExit;
        if (loopLength <= _previousLoopLength) {
            _status = HangDetectionResult::FAILED;
            return;
        }
        _previousLoopLength = loopLength;
    }
    _numStepsAtLastLoopExit = _searcher.getNumSteps();

    if (++_numLoopExits > 2) {
        checkGliderContract();
        if (_numLoopExits > 3 && _status == HangDetectionResult::ONGOING) {
            // Conclude it's a hang after two successful checks (just in case)
            _status = HangDetectionResult::HANGING;
        }
    }

    if (_status == HangDetectionResult::ONGOING) {
        _searcher.getDataTracker().captureSnapShot();
        _dataPointerAtLoopStart = nullptr;
    }
}

void GliderHangDetector::signalLoopIterationCompleted() {
    if (_dataPointerAtLoopStart == nullptr) {
        _dataPointerAtLoopStart = _searcher.getData().getDataPointer();
    }
    else if (_dataPointerAtLoopStart != _searcher.getData().getDataPointer()) {
        // Execution of the inner-loop of the glider should not result in effective DP-movements
        // (inside the loop, DP is allowed to temporarily move). Effective DP-movement should only
        // be done by the meta-loop, which causes the glider to move.
        _status = HangDetectionResult::FAILED;
    }
}

HangDetectionResult GliderHangDetector::detectHang() {
    // Actual check is done whenever a loop is exited
    return _status;
}
