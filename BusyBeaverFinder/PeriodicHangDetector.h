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

class PeriodicHangDetector : public HangDetector {

    ExhaustiveSearcher& _searcher;

    //----------------------
    // Configuration

    // The minimum number of recorded instructions before trying to find the repetition period
    int _minRecordedInstructions;

    //----------------------
    // Hang detection state

    ProgramBlock* _sampleBlock;
    int _cyclePeriod;

    int _sampleStartIndex;
    // When to perform the periodic hang check (in number of recorded instructions)
    int _periodicHangCheckAt;

    int determineCyclePeriod();

public:
    PeriodicHangDetector(ExhaustiveSearcher& searcher);

    void setMinRecordedInstructions(int val) { _minRecordedInstructions = val; }
    int getMinRecordedInstructions() { return _minRecordedInstructions; }

    HangType hangType() { return HangType::PERIODIC; }

    void start();
    HangDetectionResult detectHang();
};

#endif /* PeriodicHangDetector_h */
