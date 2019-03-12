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
    SweepHangDetector(searcher),
    _deltaTracker(searcher)
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
    _deltaTracker.reset();

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
//        _deltaTracker.dump();

        if (isSweepDiverging()) {
            int max = _deltaTracker.getMaxShr() > _deltaTracker.getMaxShl()
                ? _deltaTracker.getMaxShr()
                : _deltaTracker.getMaxShl();
            if (max * 2 + 1 > sweepCount()) {
                // This sweep contains multiple shifts in immediate succession, which could mean
                // that some values are not evaluated. Therefore, we cannot yet conclude that is a
                // hang, we need to sweep the sequence a few more times (the amount depends on the
                // maximum amount that is shifted).
                return;
            }

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

    _deltaTracker.update();

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

//    if (_deltaTracker.getMaxShr() > 1 || _deltaTracker.getMaxShl() > 1) {
//        // Moved past a data value without evaluating it. This may not be a regular sweep hang, as
//        // the skipped value might impact execution if it is encountered later.
//        return HangDetectionResult::FAILED;
//    }

    return HangDetectionResult::ONGOING;
}
