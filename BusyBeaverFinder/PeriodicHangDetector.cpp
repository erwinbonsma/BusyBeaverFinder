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

void PeriodicHangDetector::setHangDetectionResult(HangDetectionResult result) {
    _status = result;
}

bool PeriodicHangDetector::isPeriodicLoopPattern() {
    return _trackedRunSummary->isInsideLoop();
}

RunSummary* PeriodicHangDetector::getTargetRunSummary() {
    return &_searcher.getRunSummary();
}

void PeriodicHangDetector::PeriodicHangDetector::start() {
    _trackedRunSummary = getTargetRunSummary();
    _status = HangDetectionResult::ONGOING;

    if ( !isPeriodicLoopPattern() ) {
        setHangDetectionResult(HangDetectionResult::FAILED);
        return;
    }

    _loopPeriod = _trackedRunSummary->getLoopPeriod();
    _loopRunBlockIndex = _trackedRunSummary->getNumRunBlocks();
    _searcher.getDataTracker().reset();
}

void PeriodicHangDetector::captureAndCheckSnapshot() {
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
        setHangDetectionResult(
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
        setHangDetectionResult(
            dataTracker.periodicHangDetected()
            ? HangDetectionResult::HANGING
            : HangDetectionResult::FAILED
        );
    }
}

void PeriodicHangDetector::signalLoopIterationCompleted() {
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

    captureAndCheckSnapshot();
}

void PeriodicHangDetector::signalLoopExit() {
    // The assumption is that the loop is endless
    setHangDetectionResult(HangDetectionResult::FAILED);
}

HangDetectionResult PeriodicHangDetector::detectHang() {
    return _status;
}
