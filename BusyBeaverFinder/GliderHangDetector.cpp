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

    if (++_numLoopExits > 2) {
        checkGliderContract();
        if (_numLoopExits > 3 && _status == HangDetectionResult::ONGOING) {
            // Conclude it's a hang after two successful checks (just in case)
            _status = HangDetectionResult::HANGING;
        }
    }

    if (_status == HangDetectionResult::ONGOING) {
        _searcher.getDataTracker().captureSnapShot();
    }
}

HangDetectionResult GliderHangDetector::detectHang() {
    // Actual check is done whenever a loop is exited
    return _status;
}
