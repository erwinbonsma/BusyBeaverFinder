//
//  RegularSweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 09/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "RegularSweepHangDetector.h"

#include "ExhaustiveSearcher.h"
#include "Utils.h"

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
//    _searcher.getData().dump();
//    _searcher.getDataTracker().dump();

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

// Returns "true" if detection should continue
bool RegularSweepHangDetector::checkForHang() {
    if (!isSweepDiverging()) {
        // Values do not diverge so we cannot conclude it is a hang.
        _status = HangDetectionResult::FAILED;
        return false;
    }

    int max = _deltaTracker.getMaxShr() > _deltaTracker.getMaxShl()
        ? _deltaTracker.getMaxShr()
        : _deltaTracker.getMaxShl();
    if (max * 2 - 1 > sweepCount()) {
        // This sweep contains multiple shifts in immediate succession, which could mean
        // that some values are not evaluated. Therefore, we cannot yet conclude that is a
        // hang, we need to sweep the sequence a few more times (the amount depends on the
        // maximum amount that is shifted).
        return true;
    }

    Data& data = _searcher.getData();
    if (max >= (data.getMaxVisitedP() - data.getMinVisitedP())) {
        // The sequence is too short. This may be a glider instead.
        _status = HangDetectionResult::FAILED;
        return false;
    }

    ProgramPointer pp = _searcher.getProgramPointer();

    if (PROGRAM_POINTERS_MATCH(pp, _sweepStartPp)) {
        if (_searcher.atTargetProgram()) {
            _searcher.getProgram().dump();
            _searcher.getData().dump();
            _searcher.getDataTracker().dump();
        }

        // PP is same as it was at was at the "start", so this is an actual repetition/hang
        _status = HangDetectionResult::HANGING;
        return false;
    }

    // This may be a sweep with Unbalanced Growth. Continue detection
    return true;
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
//    data.dump();

    if (sweepCount() == 2 && dp > data.getMinBoundP() && dp < data.getMaxBoundP()) {
        _sweepMidTurningPoint = data.getDataPointer();
    }
    else if (sweepCount() == 3) {
        // Set the start PP only after a full sweep. This ensures that it is the instruction where
        // the first left turn occurs for this sweep. When the reveral comprises of multiple left
        // turns, it may not take the actual start PP when it would determine it during the very
        // first reversal
        _sweepStartPp = _searcher.getProgramPointer();
    }
    else if (sweepCount() % 2 == 1) {
//        std::cout << "Checking divergence" << std::endl;
//        _deltaTracker.dump();
        if (!checkForHang()) {
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

    return HangDetectionResult::ONGOING;
}
