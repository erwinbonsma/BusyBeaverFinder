//
//  SweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 10/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "SweepHangDetector.h"

#include "ExhaustiveSearcher.h"


SweepHangDetector::SweepHangDetector(ExhaustiveSearcher& searcher) :
    _searcher(searcher)
{
    // Set defaults
    _maxSweepCount = 5;
}

void SweepHangDetector::start() {
    _sweepCount = 0;
}

bool SweepHangDetector::updateSweepStatus() {
    if (!_searcher.performedTurn()) {
        return true;
    }

    if (_searcher.lastTurnWasRight()) {
        _isReversing = false;
        return (_sweepCount < _maxSweepCount);
    }

    Data& data = _searcher.getData();
    int* dp = data.getDataPointer();

    if (isStarting()) {
        if (dp <= data.getMinBoundP() || dp >= data.getMaxBoundP()) {
            // Sweep starting-point found
//            std::cout << "Sweep start found" << std::endl;
//            _searcher.dump();

            _sweepStartPp = _searcher.getProgramPointer();
            _isStartAtRight = (dp >= data.getMaxBoundP());
            _prevSweepTurnDp = dp;

            _isReversing = true;
            _movingRightwards = !_isStartAtRight; // Direction of upcoming sweep
            _sweepCount++;
        }

        return true;
    }

    if (_isReversing) {
        // No right-turn since reverse started, so this is a continuation of the same reversal
        return true;
    }

    if (dp == _prevSweepTurnDp) {
        // Although there were one or more right-turns since the previous reveral, we are still at
        // the same data location. This can happen when the reversal logic contains one or more
        // operations that cancel each other out (e.g. a left-shift towards a non-zero value
        // followed by a right-shift towards the zero reversal location). This is also considered
        // part of the same reversal.
        _isReversing = true;
        return true;
    }

    // Check adherence to sweep contract
    if (
        (_movingRightwards && dp <= _prevSweepTurnDp) ||
        (!_movingRightwards && dp >= _prevSweepTurnDp)
    ) {
        // DP continued moving in the same direction since the last expected sweep reversal
        return false;
    }

//    std::cout << "Sweep reversal" << std::endl;
//    _searcher.dump();

    _sweepCount++;
    _isReversing = true;
    _movingRightwards = !_movingRightwards;
    _prevSweepTurnDp = dp;

    return true;
}
