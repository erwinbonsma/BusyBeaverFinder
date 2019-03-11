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
#include "SweepHangDetector.h"
#include "DeltaTracker.h"

class ExhaustiveSearcher;

class RegularSweepHangDetector : public SweepHangDetector {

    //----------------------
    // Hang detection state
    HangDetectionResult _status;
    DataPointer _sweepMidTurningPoint;
    DeltaTracker _deltaTracker;

    bool isSweepDiverging();

protected:
    void sweepStarted();
    void sweepReversed();
    void sweepBroken();

public:
    RegularSweepHangDetector(ExhaustiveSearcher& searcher);

    HangType hangType() { return HangType::REGULAR_SWEEP; }

    void start();
    HangDetectionResult detectHang();
};

#endif /* RegularSweepHangDetector_h */
