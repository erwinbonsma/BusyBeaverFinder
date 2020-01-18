//
//  StaticMetaPeriodicHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 18/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef StaticMetaPeriodicHangDetector_h
#define StaticMetaPeriodicHangDetector_h

#include <stdio.h>

#include "StaticPeriodicHangDetector.h"

class StaticMetaPeriodicHangDetector : StaticPeriodicHangDetector {
    int _loopLength;

protected:
    int currentCheckPoint() { return _searcher.getMetaRunSummary().getNumRunBlocks(); }

    Trilian exhibitsHangBehaviour(bool loopContinues);

    bool analyseLoop(LoopAnalysis &loop, int &loopStart);

public:
    StaticMetaPeriodicHangDetector(ExhaustiveSearcher& searcher);
};

#endif /* StaticMetaPeriodicHangDetector_h */
