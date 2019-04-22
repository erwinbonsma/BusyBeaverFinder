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
class RunBlock;

const int maxNonSweepLoopsToIgnore = 2;

class SweepHangDetector : public HangDetector {

    ExhaustiveSearcher& _searcher;

    //----------------------
    // Hang detection state

    // The last bounds of sweep.
    DataPointer _reversalDp[2];

    // The mid-sequence reveral point, if any. If there is one, it should be fixed to zero during
    // the sweep loops
    DataPointer _midSequenceReveralDp;

    DataPointer _dataPointerAtLoopStart;
    DataPointer _dataBoundary;

    // The number of sweeps so far.
    int _sweepCount;

    // The loop in the meta-run summary (it is used to verify that we remain inside this loop)
    int _metaLoopIndex;

    // The period of the meta-loop. It's typically four but can be larger when a reversal sequence
    // contains a fixed loop.
    int _metaLoopPeriod;

    // The maximum amount DP shifts within a program block during a sweep run block.
    int _maxSweepShift;

    int _numNonSweepLoopsToIgnore;
    int _nonSweepLoopIndexToIgnore[maxNonSweepLoopsToIgnore];
    bool _ignoreCurrentLoop;

    void setHangDetectionResult(HangDetectionResult result);

    bool shouldIgnoreCurrentLoop();

    int getMaxShiftForLoop(RunBlock* runBlock);
    int determineMaxSweepShift();

    bool isSweepLoopPattern();

    bool isSweepDiverging();
    HangDetectionResult checkSweepContract();

public:
    SweepHangDetector(ExhaustiveSearcher& searcher);

    HangType hangType() { return HangType::REGULAR_SWEEP; }

    HangDetectionResult start();

    HangDetectionResult signalLoopStartDetected();
    HangDetectionResult signalLoopIteration();
    HangDetectionResult signalLoopExit();
};

#endif /* SweepHangDetector_h */
