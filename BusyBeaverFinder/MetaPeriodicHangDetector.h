//
//  MetaPeriodicHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 18/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef MetaPeriodicHangDetector_h
#define MetaPeriodicHangDetector_h

#include "PeriodicHangDetector.h"

#include <stdio.h>

// Variant of its base class, which detects hangs at the lowest-level run summary. This can instead
// detect hangs at the meta-level. These hangs occur when the periodic hang itself contains of one
// or more (fixed length) loops.
class MetaPeriodicHangDetector : public PeriodicHangDetector {
    // The units of the three variables below are all in program blocks
    int _loopLength;
    int _abortTime;
    int _timeOfNextSnapshot;

    RunSummary* getTargetRunSummary();

    bool isPeriodicLoopPattern();

public:
    MetaPeriodicHangDetector(ExhaustiveSearcher& searcher);

    void start();

    void signalLoopIterationCompleted();
    void signalLoopExit();
};

#endif /* MetaPeriodicHangDetector_h */
