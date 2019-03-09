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
    _searcher(searcher)
{
    // Set defaults
    _maxSweepExtensionCount = 5;
}

void RegularSweepHangDetector::start() {
    _extensionCount = 0;
    _prevExtensionDir = DataDirection::NONE;
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
    Data& data = _searcher.getData();
    DataTracker& dataTracker = _searcher.getDataTracker();

    // The mid-turning point should not be crossed
    if (_sweepMidTurningPoint != nullptr) {
        int* dp = data.getDataPointer();
        if (
            (_sweepMidTurningDir == DataDirection::RIGHT && dp > _sweepMidTurningPoint) ||
            (_sweepMidTurningDir == DataDirection::LEFT && dp < _sweepMidTurningPoint)
        ) {
//            std::cout << "Crossed the mid-turning point" << std::endl;
            _sweepMidTurningPoint = nullptr;
            // Start new detection attempt afresh
            return HangDetectionResult::FAILED;
        }
    }

    if (!_searcher.performedTurn()) {
        return HangDetectionResult::ONGOING;
    }

    if (_searcher.lastTurnWasRight()) {
        _midSequence = true;
        return HangDetectionResult::ONGOING;;
    }

    DataDirection extensionDir = DataDirection::NONE;

    if (data.getDataPointer() <= data.getMinBoundP()) {
        // At left end of sequence
        extensionDir = DataDirection::LEFT;
    }
    else if (data.getDataPointer() >= data.getMaxBoundP()) {
        // At right end of sequence
        extensionDir = DataDirection::RIGHT;
    }
    else if (_sweepMidTurningPoint == nullptr && _extensionCount == 1) {
        // Possible mid-sequence turning point
        _sweepMidTurningPoint = data.getDataPointer();
        _sweepMidTurningDir = (_prevExtensionDir == DataDirection::LEFT)
            ? DataDirection::RIGHT
            : DataDirection::LEFT;

        extensionDir = _sweepMidTurningDir;
    }

    if (
        extensionDir != DataDirection::NONE &&
        extensionDir != _prevExtensionDir &&
        _midSequence
    ) {
        _prevExtensionDir = extensionDir;
        _extensionCount++;
        _midSequence = false; // We're at an end of the sequence

//        std::cout << "Sweep endpoint detected #"
//        << _extensionCount
//        << std::endl;
//        _program.dump(_pp.p);
//        _data.dump();

        ProgramPointer pp = _searcher.getProgramPointer();

        if (_extensionCount == 1) {
            _sweepStartPp = pp;
        }
        else if (_extensionCount % 2 == 1) {
            if (isSweepDiverging()) {
                if (pp.p == _sweepStartPp.p && pp.dir == _sweepStartPp.dir) {
                    // PP is same as it was at during the first turn, so an actual repetition/hang
//                    std::cout << "Sweep hang detected!" << std::endl;
//                    _data.dump();
//                    _dataTracker.dump();

                    return HangDetectionResult::HANGING;
                } else {
                    // This may be a sweep with Unbalanced Growth
                    if (_extensionCount >= _maxSweepExtensionCount) {
                        return HangDetectionResult::FAILED;
                    }
                }
            } else {
                // Values do not diverge so we cannot conclude it is a hang.
                return HangDetectionResult::FAILED;
            }
        }
        dataTracker.captureSnapShot();
    }

    return HangDetectionResult::ONGOING;
}
