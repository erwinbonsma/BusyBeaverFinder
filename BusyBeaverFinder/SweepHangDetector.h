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

class SweepHangDetector : public HangDetector {

    ExhaustiveSearcher& _searcher;

    //----------------------
    // Hang detection state

    HangDetectionResult _status;

    // The last bounds of sweep.
    DataPointer _reversalDp[2];

    // The mid-sequence reveral point, if any. If there is one, it should be fixed to zero during
    // the sweep loops
    DataPointer _midSequenceReveralDp;

    // The number of sweeps so far.
    int _sweepCount;

    // The loop in the meta-run summary (it is used to verify that we remain inside this loop)
    int _metaLoopIndex;

    // The maximum amount DP shifts within a program block during a sweep run block.
    int _maxSweepShift;

    int getMaxShiftForLoop(RunBlock* runBlock);
    int determineMaxSweepShift();

    bool isSweepDiverging();
    bool verifySweepContract();

public:
    SweepHangDetector(ExhaustiveSearcher& searcher);

    HangType hangType() { return HangType::REGULAR_SWEEP; }

    void start();
    void signalLoopExit();
    HangDetectionResult detectHang();
};

#endif /* SweepHangDetector_h */
