//
//  StaticPeriodicHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef StaticPeriodicHangDetector_h
#define StaticPeriodicHangDetector_h

#include <stdio.h>

#include "StaticHangDetector.h"

#include "ExhaustiveSearcher.h"
#include "RunSummary.h"
#include "LoopAnalysis.h"

class StaticPeriodicHangDetector : public StaticHangDetector {
    LoopAnalysis _loop;
    int _loopStart;

    bool checkAllFreshlyConsumedValuesWillBeZero();

protected:
    int currentCheckPoint() { return _searcher.getRunSummary().getNumRunBlocks(); }

    // Analyses the loop. Returns "true" iff analysis was successful. It will then also have updated
    // loopStart to the point where the loop starts.
    virtual bool analyseLoop(LoopAnalysis &loop, int &loopStart);

    bool exhibitsHangBehaviour();

    HangDetectionResult tryProofHang(bool resumed);

public:
    StaticPeriodicHangDetector(ExhaustiveSearcher& searcher);

    HangType hangType() { return HangType::PERIODIC; }
};

#endif /* StaticPeriodicHangDetector_h */
