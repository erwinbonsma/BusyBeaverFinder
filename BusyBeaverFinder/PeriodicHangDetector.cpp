//
//  PeriodicHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 09/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "PeriodicHangDetector.h"

#include "ExhaustiveSearcher.h"

PeriodicHangDetector::PeriodicHangDetector(ExhaustiveSearcher& searcher) :
    _searcher(searcher) {
}

void PeriodicHangDetector::PeriodicHangDetector::start() {
    _samplePp = _searcher.getProgramPointer();

    _cyclePeriod = _searcher.getCycleDetector().getCyclePeriod();
    _searcher.getCycleDetector().clearInstructionHistory();
    _periodicHangCheckAt = _cyclePeriod;
//    std::cout << "period = " << _cyclePeriod << std::endl;

    _searcher.getData().resetHangDetection();
    _searcher.getDataTracker().reset();
    _searcher.getDataTracker().captureSnapShot();
}

HangDetectionResult PeriodicHangDetector::detectHang() {
    if (_searcher.getCycleDetector().getNumRecordedInstructions() < _periodicHangCheckAt) {
        return HangDetectionResult::ONGOING;
    }

    ProgramPointer pp = _searcher.getProgramPointer();
    if (pp.p != _samplePp.p || pp.dir != _samplePp.dir) {
        // Not back at the sample point, so not on a hang cycle with assumed period.
        return HangDetectionResult::FAILED;
    }

    Data& data = _searcher.getData();
    DataTracker& dataTracker = _searcher.getDataTracker();

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
            _periodicHangCheckAt = _cyclePeriod * 2;
        }

        dataTracker.captureSnapShot();
    }

    return HangDetectionResult::ONGOING;
}
