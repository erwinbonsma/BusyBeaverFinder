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

void SweepHangDetector::signalLeftTurn() {
    Data& data = _searcher.getData();
    int* dp = data.getDataPointer();

    if (_sweepCount == 0) {
        if (dp <= data.getMinBoundP() || dp >= data.getMaxBoundP()) {
            // Sweep starting-point found
//            std::cout << "Sweep start found" << std::endl;
//            _searcher.dump();

            _isStartAtRight = (dp >= data.getMaxBoundP());
            _prevSweepTurnDp = dp;
            if (_isStartAtRight) {
                _rightReversalDp = dp;
            } else {
                _leftReversalDp = dp;
            }
            _movingRightwards = !_isStartAtRight; // Direction of upcoming sweep
            _sweepCount++;

            sweepStarted();
        }

        return;
    }

    if (dp == _prevSweepTurnDp) {
        // Although there were one or more right-turns since the previous reveral, we are still at
        // the same data location. This can happen when the reversal logic contains one or more
        // operations that cancel each other out (e.g. a left-shift towards a non-zero value
        // followed by a right-shift towards the zero reversal location). This is also considered
        // part of the same reversal.
        return;
    }

    // Check adherence to sweep contract
    if (_movingRightwards) {
        if (
            // Continued beyond expected sweep reversal point?
            dp <= _prevSweepTurnDp ||
            // Premature turn?
            (_sweepCount >= 2 && dp < _rightReversalDp)
        ) {
            sweepBroken();
            return;
        }

    } else {
        if (
            // Continued beyond expected sweep reversal point?
            dp >= _prevSweepTurnDp ||
            // Premature turn?
            (_sweepCount >= 2 && dp > _leftReversalDp)
        ) {
            sweepBroken();
            return;
        }
    }

//    std::cout << "Sweep reversal" << std::endl;

    _sweepCount++;
    if (_movingRightwards) {
        _rightReversalDp = dp;
    } else {
        _leftReversalDp = dp;
    }
    _movingRightwards = !_movingRightwards;
    _prevSweepTurnDp = dp;

    sweepReversed();
}
