//
//  StaticGliderHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef StaticGliderHangDetector_h
#define StaticGliderHangDetector_h

#include "StaticHangDetector.h"

#include "LoopAnalysis.h"

const int maxAheadOffset = 4;

class StaticGliderHangDetector : public StaticHangDetector {

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
    StaticGliderHangDetector(const ProgramExecutor& executor);

    virtual HangType hangType() { return HangType::APERIODIC_GLIDER; }
};

#endif /* StaticGliderHangDetector_h */
