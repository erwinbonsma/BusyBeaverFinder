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

    LoopAnalysis _loop;
    RunBlock* _loopRunBlock;

    bool isGliderLoop(int &currentCounterDpOffset, int &nextCounterDpOffset);

    bool exitingAtLoopCounterChange(int currentCounterDpOffset);

    bool transitionChangesLoopCounter(int curCounterDpOffset, int nxtCounterDpOffset);

    bool onlyZeroesAhead(int dpShift);

protected:
    bool shouldCheckNow(bool loopContinues);

    bool analyzeHangBehaviour();

    Trilian proofHang();

public:
    StaticGliderHangDetector(ExhaustiveSearcher& searcher);

    virtual HangType hangType() { return HangType::APERIODIC_GLIDER; }
};

#endif /* StaticGliderHangDetector_h */
