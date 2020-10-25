//
//  GliderHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef GliderHangDetector_h
#define GliderHangDetector_h

#include "HangDetector.h"

#include "LoopAnalysis.h"

const int maxAheadOffset = 4;

class GliderHangDetector : public HangDetector {

    SequenceAnalysis _transitionSequence;
    LoopAnalysis _loop;
    const RunBlock* _loopRunBlock;
    int _aheadDelta[maxAheadOffset];

    int _curCounterDpOffset, _curCounterDelta;
    int _nxtCounterDpOffset, _nxtCounterDelta;
    int _numBootstrapCycles;

    // Loop analysis
    bool identifyLoopCounter();
    bool isGliderLoop();
    bool analyzeLoop();

    // Sequence analysis
    bool checkTransitionDeltas();
    bool analyzeTransitionSequence();

    // Dynamic checks
    bool isBootstrapping();
    bool transitionSequenceIsFixed();
    bool onlyZeroesAhead();

protected:
    bool shouldCheckNow(bool loopContinues);

    bool analyzeHangBehaviour();

    Trilian proofHang();

public:
    GliderHangDetector(const ProgramExecutor& executor);

    virtual HangType hangType() { return HangType::APERIODIC_GLIDER; }
};

#endif /* GliderHangDetector_h */
