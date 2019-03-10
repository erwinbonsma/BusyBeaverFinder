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
    _status = HangDetectionResult::ONGOING;
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

void RegularSweepHangDetector::sweepStarted() {
    _searcher.getDataTracker().captureSnapShot();
    _lastTurnDp = _searcher.getLastTurnDataPointer();

//    std::cout << "Sweep started" << std::endl;
//    _searcher.dump();
}

void RegularSweepHangDetector::sweepReversed() {
    Data& data = _searcher.getData();
    int* dp = data.getDataPointer();

//    std::cout << "Sweep reversed" << std::endl;
//    _searcher.dump();

    if (sweepCount() == 2 && dp > data.getMinBoundP() && dp < data.getMaxBoundP()) {
        _sweepMidTurningPoint = data.getDataPointer();
    }
    else if (sweepCount() % 2 == 1) {
//        std::cout << "Checking divergence" << std::endl;
        if (isSweepDiverging()) {
            ProgramPointer pp = _searcher.getProgramPointer();
            ProgramPointer startPp = sweepStartProgramPointer();

            if (pp.p == startPp.p && pp.dir == startPp.dir) {
                // PP is same as it was at during the first turn, so an actual repetition/hang
                _status = HangDetectionResult::HANGING;
                return;
            } else {
                // This may be a sweep with Unbalanced Growth. Continue detection
            }
        } else {
            // Values do not diverge so we cannot conclude it is a hang.
            _status = HangDetectionResult::FAILED;
            return;
        }
    }

    _searcher.getDataTracker().captureSnapShot();
    _lastTurnDp = _searcher.getLastTurnDataPointer();
}

void RegularSweepHangDetector::sweepBroken() {
    _status = HangDetectionResult::FAILED;

//    std::cout << "Sweep Broken" << std::endl;
//    _searcher.dump();
}

HangDetectionResult RegularSweepHangDetector::detectHang() {
    if (_status != HangDetectionResult::ONGOING) {
        return _status;
    }
    if (sweepCount() == 0) {
        return HangDetectionResult::ONGOING;
    }

    // The mid-turning point should not be crossed
    if (_sweepMidTurningPoint != nullptr) {
        int* dp = _searcher.getData().getDataPointer();

        if (
            (isStartAtRight() && dp < _sweepMidTurningPoint) ||
            (!isStartAtRight() && dp > _sweepMidTurningPoint)
        ) {
            // Crossed the mid-sweep turning point which breaks the assumption that it is a fixed
            // turning point.
            return HangDetectionResult::FAILED;
        }
    }

    if (
        (movingRightwards() && (_searcher.getLastTurnDataPointer() - _lastTurnDp) > 1) ||
        (!movingRightwards() && (_lastTurnDp - _searcher.getLastTurnDataPointer()) > 1)
    ) {
        // Moved past a data value without evaluating it. This may not be a regular sweep hang, as
        // the skipped value might impact execution if it is encountered later.
        return HangDetectionResult::FAILED;
    }
    _lastTurnDp = _searcher.getLastTurnDataPointer();

    return HangDetectionResult::ONGOING;
}
