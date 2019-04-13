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
    _periodicHangCheckAt =
        _searcher.getRunSummary().getNumProgramBlocks() + _minRecordedProgramBlocks;
}

bool PeriodicHangDetector::insideLoop() {
    RunSummary& runSummary = _searcher.getRunSummary();

    if (!runSummary.isInsideLoop()) {
        // Not in a loop (yet)
        return false;
    }

    _loopPeriod = runSummary.getLoopPeriod();
    _loopRunBlockIndex = runSummary.getNumRunBlocks();
    _periodicHangCheckAt = runSummary.getNumProgramBlocks() + _loopPeriod;

//    std::cout << "loop period = " << _loopPeriod << std::endl;
//    _searcher.dump();
//    runSummary.dump();

    _searcher.getDataTracker().reset();
    _searcher.getDataTracker().captureSnapShot();
    _searcher.getData().resetHangDetection();

    return true;
}

HangDetectionResult PeriodicHangDetector::detectHang() {
    RunSummary& runSummary = _searcher.getRunSummary();

    if (runSummary.getNumProgramBlocks() < _periodicHangCheckAt) {
        return HangDetectionResult::ONGOING;
    }

    if (_loopPeriod == 0) {
        return insideLoop() ? HangDetectionResult::ONGOING : HangDetectionResult::FAILED;
    }

    if (!runSummary.isInsideLoop() || _loopRunBlockIndex != runSummary.getNumRunBlocks()) {
        // Apparently not same periodic loop anymore
        return HangDetectionResult::FAILED;
    }

    Data& data = _searcher.getData();
    DataTracker& dataTracker = _searcher.getDataTracker();

//    _searcher.dump();
//    runSummary.dump();
//    dataTracker.dump();

    if (
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
        if (dataTracker.getOldSnapShot() != nullptr) {
            if (dataTracker.periodicHangDetected()) {
                return HangDetectionResult::HANGING;
            } else {
                return HangDetectionResult::FAILED;
            }
        } else {
            _periodicHangCheckAt = runSummary.getNumProgramBlocks() + _loopPeriod;
        }

        dataTracker.captureSnapShot();
    }

    return HangDetectionResult::ONGOING;
}
