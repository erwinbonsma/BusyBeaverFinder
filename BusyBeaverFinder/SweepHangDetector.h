//
//  SweepHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 10/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef SweepHangDetector_h
#define SweepHangDetector_h

#include <stdio.h>

#include "Types.h"
#include "HangDetector.h"

class ExhaustiveSearcher;

class SweepHangDetector : public HangDetector {

    // Configuration
    int _maxSweepCount;

    //----------------------
    // Hang detection state

    // PP at start of the sweep. It is always at one end of the sequence (i.e. at a data bound).
    ProgramPointer _sweepStartPp;

    // Indicates if the sweep starts at the right or left of the data sequence.
    bool _isStartAtRight;

    // DP at moment of last sweep reversal (which was then zero, triggering a left turn). It is
    // used to check if the program actually carries out a sweep.
    int* _prevSweepTurnDp;

    // True when a sweep reversal is in progress. When reversing, there may be multiple left-turns
    // based on the same data value. These are all considered part of the same reversal.
    bool _isReversing;

    // The direction of the current sweep (or upcoming sweep, in case a turn is in progress)
    bool _movingRightwards;

    // The number of sweeps so far. It counts the
    int _sweepCount;

protected:
    ExhaustiveSearcher& _searcher;

    ProgramPointer sweepStartProgramPointer() { return _sweepStartPp; }
    int sweepCount() { return _sweepCount; }

    // Returns "true" while the sweep start point has not yet been set.
    bool isStarting() { return _sweepCount == 0; }

    bool isStartAtRight() { return _isStartAtRight; }

    bool isReversing() { return _isReversing; }

    bool isMovingRightwards() { return _movingRightwards; }

    // Updates sweep status. Returns "false" when the program does not execute a sweep. This is the
    // case when after an expected change of sweep direction, DP continued moving in the same
    // direction. The hang detection should be aborted.
    bool updateSweepStatus();

public:
    SweepHangDetector(ExhaustiveSearcher& searcher);

    void setMaxSweepCount(int val) { _maxSweepCount = val; }

    virtual void start();
};

#endif /* SweepHangDetector_h */
