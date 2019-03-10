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

class ExhaustiveSearcher;

class RegularSweepHangDetector : public SweepHangDetector {

    // Hang detection state
    HangDetectionResult _status;
    DataPointer _sweepMidTurningPoint;

    bool isSweepDiverging();

protected:
    void sweepStarted();
    void sweepReversed();
    void sweepBroken();

public:
    RegularSweepHangDetector(ExhaustiveSearcher& searcher);

    void start();
    HangDetectionResult detectHang();
};

#endif /* RegularSweepHangDetector_h */
