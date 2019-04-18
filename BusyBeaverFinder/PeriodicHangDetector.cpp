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

void PeriodicHangDetector::PeriodicHangDetector::start() {
    _loopPeriod = 0;
    _trackedRunSummary = &_searcher.getRunSummary();
    _periodicHangCheckAt = 0;
}

RunSummary* PeriodicHangDetector::getTargetRunSummary() {
    return &_searcher.getRunSummary();
}

bool PeriodicHangDetector::insideLoop() {
    if (getTargetRunSummary()->isInsideLoop()) {
        _trackedRunSummary = getTargetRunSummary();
    } else {
        // Not in a loop (yet)
        return false;
    }

    _loopPeriod = _trackedRunSummary->getLoopPeriod();
    _loopRunBlockIndex = _trackedRunSummary->getNumRunBlocks();
    // Ensure that first snapshot is taken when program block is entered. In case of the meta-run
    // summary, this method may not be invoked at the start of a meta-run program block.
    _periodicHangCheckAt = _trackedRunSummary->getNumProgramBlocks() + 1;

//    std::cout << "loop period = " << _loopPeriod << std::endl;
//    _searcher.dump();
//    _searcher.dumpHangDetection();

    _searcher.getDataTracker().reset();

    return true;
}

HangDetectionResult PeriodicHangDetector::detectHang() {
    if (_trackedRunSummary->getNumProgramBlocks() < _periodicHangCheckAt) {
        return HangDetectionResult::ONGOING;
    }

    if (_loopPeriod == 0) {
        return insideLoop() ? HangDetectionResult::ONGOING : HangDetectionResult::FAILED;
    }

    if (
        !_trackedRunSummary->isInsideLoop() ||
        _loopRunBlockIndex != _trackedRunSummary->getNumRunBlocks()
    ) {
        // Apparently not same periodic loop anymore
        return HangDetectionResult::FAILED;
    }

    // TODO: In case of a loop at the meta-run level should add an extra sanity-check that verifies
    // that each iteration contains a fixed number of program blocks (i.e. the number of iterations
    // in the lower-level loop is fixed, in contrast to sweep hangs).

    Data& data = _searcher.getData();
    DataTracker& dataTracker = _searcher.getDataTracker();

//    _searcher.dump();
//    _searcher.dumpHangDetection();

    if (
        dataTracker.getNewSnapShot() == nullptr
    ) {
        _searcher.getDataTracker().captureSnapShot();
        _searcher.getData().resetHangDetection();

        _periodicHangCheckAt = _trackedRunSummary->getNumProgramBlocks() + _loopPeriod;
    }
    else if (
        data.getDataPointer() == dataTracker.getNewSnapShot()->dataP &&
        !data.significantValueChange()
    ) {
        SnapShotComparison result = dataTracker.compareToSnapShot();
        if (result != SnapShotComparison::IMPACTFUL) {
            return HangDetectionResult::HANGING;
        } else {
            return HangDetectionResult::FAILED;
        }
    }
    else {
        if (dataTracker.getOldSnapShot() == nullptr) {
            _periodicHangCheckAt = _trackedRunSummary->getNumProgramBlocks() + _loopPeriod;
        } else {
            if (dataTracker.periodicHangDetected()) {
                return HangDetectionResult::HANGING;
            } else {
                return HangDetectionResult::FAILED;
            }
        }

        dataTracker.captureSnapShot();
    }

    return HangDetectionResult::ONGOING;
}
