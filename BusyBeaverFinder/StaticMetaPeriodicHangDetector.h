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

class StaticMetaPeriodicHangDetector : public StaticPeriodicHangDetector {
protected:
    bool shouldCheckNow(bool loopContinues);
    bool analyzeHangBehaviour();

public:
    StaticMetaPeriodicHangDetector(ExhaustiveSearcher& searcher);
};

#endif /* StaticMetaPeriodicHangDetector_h */
