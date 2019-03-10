//
//  RegularSweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 09/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "RegularSweepHangDetector.h"

#include "ExhaustiveSearcher.h"

RegularSweepHangDetector::RegularSweepHangDetector(ExhaustiveSearcher& searcher) :
    SweepHangDetector(searcher)
{
}

void RegularSweepHangDetector::start() {
    SweepHangDetector::start();

    _sweepMidTurningPoint = nullptr;
}

// Should be invoked after a full sweep has been completed. It returns "true" if the data value
// changes are diverging and therefore, the program is considered hanging.
bool RegularSweepHangDetector::isSweepDiverging() {
//    std::cout << "Checking for sweep hang " << std::endl;
//    _data.dump();
//    _dataTracker.dump();

    Data& data = _searcher.getData();
    DataTracker& dataTracker = _searcher.getDataTracker();

    if (_sweepMidTurningPoint != nullptr) {
        long dataIndex = _sweepMidTurningPoint - data.getDataBuffer();
        if (
            *_sweepMidTurningPoint != dataTracker.getOldSnapShot()->buf[dataIndex]
        ) {
            // The mid-turning point should be a fixed value (when at other side of the sequence)
            return false;
        }
    }

    return dataTracker.sweepHangDetected(_sweepMidTurningPoint);
}

HangDetectionResult RegularSweepHangDetector::detectHang() {
    int prevSweepCount = sweepCount();

    if (!updateSweepStatus()) {
        return HangDetectionResult::FAILED;
    }

    Data& data = _searcher.getData();
    DataTracker& dataTracker = _searcher.getDataTracker();
    int* dp = data.getDataPointer();

    // The mid-turning point should not be crossed
    if (_sweepMidTurningPoint != nullptr) {
        if (
            (isStartAtRight() && dp < _sweepMidTurningPoint) ||
            (!isStartAtRight() && dp > _sweepMidTurningPoint)
        ) {
//            std::cout << "Crossed the mid-turning point" << std::endl;
            return HangDetectionResult::FAILED;
        }
    }

    if (sweepCount() > prevSweepCount) {
        ProgramPointer pp = _searcher.getProgramPointer();

        if (sweepCount() == 2 && dp > data.getMinBoundP() && dp < data.getMaxBoundP()) {
            _sweepMidTurningPoint = data.getDataPointer();
        }

        if (sweepCount() > 1 && sweepCount() % 2 == 1) {
            if (isSweepDiverging()) {
                ProgramPointer startPp = sweepStartProgramPointer();

                if (pp.p == startPp.p && pp.dir == startPp.dir) {
                    // PP is same as it was at during the first turn, so an actual repetition/hang
//                    std::cout << "Sweep hang detected!" << std::endl;
//                    _data.dump();
//                    _dataTracker.dump();

                    return HangDetectionResult::HANGING;
                } else {
                    // This may be a sweep with Unbalanced Growth. Continue detection
                }
            } else {
                // Values do not diverge so we cannot conclude it is a hang.
                return HangDetectionResult::FAILED;
            }
        }
        dataTracker.captureSnapShot();

        if (*(pp.p) == Ins::DATA && (pp.dir == Dir::UP || pp.dir == Dir::DOWN)) {
            // Restore the data value that caused the reveral to be zero in the snapshot. It is
            // non-zero now, as the same step that executed the left turn also executed an INC or
            // DEC operation.
            //
            // It is cleared so that DataTracker::isSweepDiverging() ignores this value.
            SnapShot* snapShot = dataTracker.getNewSnapShot();
            snapShot->buf[dp - data.getDataBuffer()] = 0;
        }
    }

    return HangDetectionResult::ONGOING;
}
