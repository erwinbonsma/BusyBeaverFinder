//
//  MetaPeriodicHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 18/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef MetaPeriodicHangDetector_h
#define MetaPeriodicHangDetector_h

#include "PeriodicHangDetector.h"

class MetaPeriodicHangDetector : public PeriodicHangDetector {
    int _metaLoopStart;

protected:
    bool shouldCheckNow(bool loopContinues);
    bool analyzeHangBehaviour();
    Trilian proofHang();

public:
    MetaPeriodicHangDetector(const ProgramExecutor& executor);

    void reset();
};

#endif /* MetaPeriodicHangDetector_h */
