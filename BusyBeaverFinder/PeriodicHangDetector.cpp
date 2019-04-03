//
//  PeriodicHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 09/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "PeriodicHangDetector.h"

#include "Utils.h"
#include "ExhaustiveSearcher.h"

PeriodicHangDetector::PeriodicHangDetector(ExhaustiveSearcher& searcher) :
    _searcher(searcher) {
}

void PeriodicHangDetector::PeriodicHangDetector::start() {
    _cyclePeriod = 0;
    _sampleStartIndex = _searcher.getCycleDetector().getNumRecordedInstructions();
    _periodicHangCheckAt = _sampleStartIndex + _minRecordedInstructions;
}

int PeriodicHangDetector::determineCyclePeriod() {
    int period = _searcher.getCycleDetector().getCyclePeriod(_sampleStartIndex);

//    std::cout << "cycle period = " << _cyclePeriod << std::endl;
//    _searcher.getCycleDetector().dump();

    _sampleBlock = _searcher.getProgramBlock();

    _searcher.getDataTracker().reset();
    _searcher.getDataTracker().captureSnapShot();
    _searcher.getData().resetHangDetection();

    return period;
}

HangDetectionResult PeriodicHangDetector::detectHang() {
    CycleDetector& cycleDetector = _searcher.getCycleDetector();

    if (cycleDetector.getNumRecordedInstructions() < _periodicHangCheckAt) {
        return HangDetectionResult::ONGOING;
    }

    if (_cyclePeriod == 0) {
        _cyclePeriod = determineCyclePeriod();
        _periodicHangCheckAt = cycleDetector.getNumRecordedInstructions() + _cyclePeriod;
        return HangDetectionResult::ONGOING;
    }

    if (_searcher.getProgramBlock() != _sampleBlock) {
        // Not back at the sample point, so not on a hang cycle with assumed period.
        return HangDetectionResult::FAILED;
    }

    if (_searcher.getCycleDetector().getCyclePeriod(_sampleStartIndex) != _cyclePeriod) {
        // Apparently not same periodic loop anymore
        return HangDetectionResult::FAILED;
    }

    Data& data = _searcher.getData();
    DataTracker& dataTracker = _searcher.getDataTracker();

//    _searcher.getProgram().dump();
//    data.dump();
//    dataTracker.dump();

    if (
        data.getDataPointer() == dataTracker.getNewSnapShot()->dataP &&
        !data.significantValueChange()
    ) {
        SnapShotComparison result = dataTracker.compareToSnapShot();
        if (result != SnapShotComparison::IMPACTFUL) {
            return HangDetectionResult::HANGING;
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
            _periodicHangCheckAt = cycleDetector.getNumRecordedInstructions() + _cyclePeriod;
        }

        dataTracker.captureSnapShot();
    }

    return HangDetectionResult::ONGOING;
}
