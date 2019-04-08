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

    // The minimum number of recorded program blocks before trying to find the repetition period
    int _minRecordedProgramBlocks;

    //----------------------
    // Hang detection state

    int _loopPeriod;
    int _loopRunBlockIndex;

    int _sampleStartIndex;
    // When to perform the periodic hang check (in number of recorded instructions)
    int _periodicHangCheckAt;

    bool insideLoop();

public:
    PeriodicHangDetector(ExhaustiveSearcher& searcher);

    void setMinRecordedProgramBlocks(int val) { _minRecordedProgramBlocks = val; }
    int getMinRecordedProgramBlocks() { return _minRecordedProgramBlocks; }

    HangType hangType() { return HangType::PERIODIC; }

    void start();
    HangDetectionResult detectHang();
};

#endif /* PeriodicHangDetector_h */
