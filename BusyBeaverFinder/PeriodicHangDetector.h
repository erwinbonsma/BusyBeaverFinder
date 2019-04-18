//
//  PeriodicHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 09/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef PeriodicHangDetector_h
#define PeriodicHangDetector_h

#include <stdio.h>

#include "Types.h"
#include "HangDetector.h"

class ExhaustiveSearcher;
class ProgramBlock;
class RunSummary;

class PeriodicHangDetector : public HangDetector {

protected:
    ExhaustiveSearcher& _searcher;

    //----------------------
    // Hang detection state

    RunSummary* _trackedRunSummary;
    int _loopPeriod;
    int _loopRunBlockIndex;

    int _sampleStartIndex;

    // When to perform the periodic hang check (in number of recorded instructions for the
    // tracked run summary)
    int _periodicHangCheckAt;

    bool insideLoop();

    // The run summary to monitor for periodic loops
    virtual RunSummary* getTargetRunSummary();

public:
    PeriodicHangDetector(ExhaustiveSearcher& searcher);

    HangType hangType() { return HangType::PERIODIC; }

    void start();
    HangDetectionResult detectHang();
};

#endif /* PeriodicHangDetector_h */
