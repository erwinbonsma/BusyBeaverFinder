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
    DataPointer _prevSweepTurnDp;

    // The current bounds of sweep. They are used to check that the sweep area does not shrink
    DataPointer _leftReversalDp, _rightReversalDp;

    // The last value that impacted a turn
    DataPointer _lastTurnDp;

    // The direction of the current sweep (or upcoming sweep, in case a turn is in progress)
    bool _movingRightwards;

    // The number of sweeps so far.
    int _sweepCount;

protected:
    ExhaustiveSearcher& _searcher;

    ProgramPointer sweepStartProgramPointer() { return _sweepStartPp; }
    int sweepCount() { return _sweepCount; }

    bool isStartAtRight() { return _isStartAtRight; }

    // Invoked when the sweep started (this is always at one end of the data sequence)
    virtual void sweepStarted() = 0;

    // Invoked when the sweep reversed
    virtual void sweepReversed() = 0;

    // Invoked when the assumed sweep was not a sweep. The hang detection should be aborted.
    virtual void sweepBroken() = 0;

public:
    SweepHangDetector(ExhaustiveSearcher& searcher);

    void setMaxSweepCount(int val) { _maxSweepCount = val; }

    virtual void start();
    void signalLeftTurn();
};

#endif /* SweepHangDetector_h */
