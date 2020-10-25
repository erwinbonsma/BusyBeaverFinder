//
//  StaticMetaPeriodicHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 18/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef StaticMetaPeriodicHangDetector_h
#define StaticMetaPeriodicHangDetector_h

#include "StaticPeriodicHangDetector.h"

class StaticMetaPeriodicHangDetector : public StaticPeriodicHangDetector {
    int _metaLoopStart;

protected:
    bool shouldCheckNow(bool loopContinues);
    bool analyzeHangBehaviour();
    Trilian proofHang();

public:
    StaticMetaPeriodicHangDetector(const ProgramExecutor& executor);

    void reset();
};

#endif /* StaticMetaPeriodicHangDetector_h */
