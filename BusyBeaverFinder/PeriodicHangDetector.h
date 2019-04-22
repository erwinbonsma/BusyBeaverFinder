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

    HangDetectionResult _status;

    RunSummary* _trackedRunSummary;
    int _loopPeriod;
    int _loopRunBlockIndex;

    void setHangDetectionResult(HangDetectionResult result);

    void captureAndCheckSnapshot();

    virtual bool isPeriodicLoopPattern();

    // The run summary to monitor for periodic loops
    virtual RunSummary* getTargetRunSummary();

public:
    PeriodicHangDetector(ExhaustiveSearcher& searcher);

    HangType hangType() { return HangType::PERIODIC; }

    void start();
    HangDetectionResult detectHang();

    void signalLoopIterationCompleted();
    void signalLoopExit();
};

#endif /* PeriodicHangDetector_h */
