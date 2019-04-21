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
    // When to abort the periodic hang check (in number of recorded instructions for the lowest
    // level run summary)
    int _abortHangCheckAt;

    RunSummary* getTargetRunSummary();
    bool insideLoop();

public:
    MetaPeriodicHangDetector(ExhaustiveSearcher& searcher);

    void start();
    HangDetectionResult detectHang();
};

#endif /* MetaPeriodicHangDetector_h */
