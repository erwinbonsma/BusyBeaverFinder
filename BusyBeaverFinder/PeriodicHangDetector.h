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

class PeriodicHangDetector : public HangDetector {

    ExhaustiveSearcher& _searcher;

    ProgramPointer _samplePp;
    int _cyclePeriod;
    // When to perform the periodic hang check (in number of recorded instructions)
    int _periodicHangCheckAt;

public:
    PeriodicHangDetector(ExhaustiveSearcher& searcher);

    HangType hangType() { return HangType::PERIODIC; }

    void start();
    HangDetectionResult detectHang();
};

#endif /* PeriodicHangDetector_h */
