//
//  PeriodicHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef PeriodicHangDetector_h
#define PeriodicHangDetector_h

#include "HangDetector.h"

#include "ExhaustiveSearcher.h"
#include "RunSummary.h"
#include "LoopAnalysis.h"

class PeriodicHangDetector : public HangDetector {

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
    PeriodicHangDetector(const ProgramExecutor& executor);

    HangType hangType() const { return HangType::PERIODIC; }

    void reset();
};

#endif /* PeriodicHangDetector_h */
