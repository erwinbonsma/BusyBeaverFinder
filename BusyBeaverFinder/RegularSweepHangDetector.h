//
//  RegularSweepHangDetector.hpp
//  BusyBeaverFinder
//
//  Created by Erwin on 09/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef RegularSweepHangDetector_h
#define RegularSweepHangDetector_h

#include <stdio.h>

#include "Types.h"
#include "HangDetector.h"

class ExhaustiveSearcher;

enum class DataDirection : char {
    NONE = 0,
    LEFT = 1,
    RIGHT = 2
};

class RegularSweepHangDetector : public HangDetector {

    ExhaustiveSearcher& _searcher;

    // Configuration
    int _maxSweepExtensionCount;

    // Hang detection state
    int _remainingSweepHangDetectAttempts;
    DataDirection _prevExtensionDir;
    int _extensionCount;
    int* _sweepMidTurningPoint;
    DataDirection _sweepMidTurningDir;
    ProgramPointer _sweepStartPp;
    bool _midSequence;

    bool isSweepDiverging();

public:
    RegularSweepHangDetector(ExhaustiveSearcher& searcher);

    void setMaxSweepExtensionCount(int val) { _maxSweepExtensionCount = val; }

    void start();
    HangDetectionResult detectHang();
};

#endif /* RegularSweepHangDetector_h */
