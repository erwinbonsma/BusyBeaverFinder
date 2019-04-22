//
//  PeriodicHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 09/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "PeriodicHangDetector.h"

#include <iostream>

#include "Utils.h"
#include "ExhaustiveSearcher.h"

PeriodicHangDetector::PeriodicHangDetector(ExhaustiveSearcher& searcher) :
    _searcher(searcher) {
}

bool PeriodicHangDetector::isPeriodicLoopPattern() {
    return _trackedRunSummary->isInsideLoop();
}

RunSummary* PeriodicHangDetector::getTargetRunSummary() {
    return &_searcher.getRunSummary();
}

HangDetectionResult PeriodicHangDetector::PeriodicHangDetector::start() {
    _trackedRunSummary = getTargetRunSummary();

    if ( !isPeriodicLoopPattern() ) {
        return HangDetectionResult::FAILED;
    }

    _loopPeriod = _trackedRunSummary->getLoopPeriod();
    _loopRunBlockIndex = _trackedRunSummary->getNumRunBlocks();
    _searcher.getDataTracker().reset();

    return HangDetectionResult::ONGOING;
}

HangDetectionResult PeriodicHangDetector::captureAndCheckSnapshot() {
    Data& data = _searcher.getData();
    DataTracker& dataTracker = _searcher.getDataTracker();

    if (
        dataTracker.getNewSnapShot() == nullptr
    ) {
        // Capture first snapshot
        dataTracker.captureSnapShot();
        data.resetHangDetection();
    }
    else if (
         data.getDataPointer() == dataTracker.getNewSnapShot()->dataP &&
         !data.significantValueChange()
    ) {
        // DP remains stationary. Check for hang against single snapshot
        return (
            dataTracker.compareToSnapShot() != SnapShotComparison::IMPACTFUL
            ? HangDetectionResult::HANGING
            : HangDetectionResult::FAILED
        );
    }
    else if (dataTracker.getOldSnapShot() == nullptr) {
        // Capture second snapshot
        dataTracker.captureSnapShot();
    }
    else {
        // Check for hang using both snapshots
        return (
            dataTracker.periodicHangDetected()
            ? HangDetectionResult::HANGING
            : HangDetectionResult::FAILED
        );
    }

    return HangDetectionResult::ONGOING;
}

HangDetectionResult PeriodicHangDetector::signalLoopIteration() {
    if (
        !_trackedRunSummary->isInsideLoop() ||
        _loopRunBlockIndex != _trackedRunSummary->getNumRunBlocks()
    ) {
        // Apparently not same periodic loop anymore
        return HangDetectionResult::FAILED;
    }

    return captureAndCheckSnapshot();
}

HangDetectionResult PeriodicHangDetector::signalLoopExit() {
    // The loop should be endless
    return HangDetectionResult::FAILED;
}
