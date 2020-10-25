//
//  StaticPeriodicHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef StaticPeriodicHangDetector_h
#define StaticPeriodicHangDetector_h

#include "StaticHangDetector.h"

#include "ExhaustiveSearcher.h"
#include "RunSummary.h"
#include "LoopAnalysis.h"

class StaticPeriodicHangDetector : public StaticHangDetector {

    bool allValuesToBeConsumedAreBeZero();

    int _loopStartLastProof;
    int _proofPhase;
    int _targetLoopLen;

protected:
    LoopAnalysis _loop;
    int _loopStart;

    bool shouldCheckNow(bool loopContinues);

    // Analyses the loop. Returns YES if it exhibits periodic hang behavior. In that case, _loop
    // and _loopStart should point to the analyzed periodic loop and its starting point.
    bool analyzeHangBehaviour();

    Trilian proofHangPhase1();
    Trilian proofHangPhase2();

    Trilian proofHang();

public:
    StaticPeriodicHangDetector(const ProgramExecutor& executor);

    HangType hangType() { return HangType::PERIODIC; }

    void reset();
};

#endif /* StaticPeriodicHangDetector_h */
