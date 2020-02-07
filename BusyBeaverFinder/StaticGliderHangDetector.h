//
//  StaticGliderHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef StaticGliderHangDetector_h
#define StaticGliderHangDetector_h

#include <stdio.h>

#include "StaticHangDetector.h"

//#include "ExhaustiveSearcher.h"
//#include "RunSummary.h"
#include "LoopAnalysis.h"

class StaticGliderHangDetector : public StaticHangDetector {

    SequenceAnalysis _transitionSequence;
    LoopAnalysis _loop;
    RunBlock* _loopRunBlock;

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

    //bool transitionChangesLoopCounter();

    // Dynamic checks
    bool isBootstrapping();
    bool transitionSequenceIsFixed();
    bool onlyZeroesAhead();

protected:
    bool shouldCheckNow(bool loopContinues);

    bool analyzeHangBehaviour();

    Trilian proofHang();

public:
    StaticGliderHangDetector(ExhaustiveSearcher& searcher);

    virtual HangType hangType() { return HangType::APERIODIC_GLIDER; }
};

#endif /* StaticGliderHangDetector_h */
