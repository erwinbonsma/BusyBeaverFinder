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

class StaticPeriodicHangDetector : public StaticHangDetector {
    int currentCheckPoint() { return _searcher.getRunSummary().getNumRunBlocks(); }

    bool exhibitsHangBehaviour();

    bool canProofHang();

public:
    HangType hangType() { return HangType::PERIODIC; }
};

#endif /* StaticPeriodicHangDetector_h */
