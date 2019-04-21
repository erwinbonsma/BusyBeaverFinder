//
//  GliderHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 14/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef GliderHangDetector_h
#define GliderHangDetector_h

#include <stdio.h>

#include "Types.h"
#include "HangDetector.h"

class ExhaustiveSearcher;
class RunBlock;

class GliderHangDetector : public HangDetector {

    ExhaustiveSearcher& _searcher;

    //----------------------
    // Hang detection state

    HangDetectionResult _status;

    // The loop in the meta-run summary (it is used to verify that we remain inside this loop)
    int _metaLoopIndex;

    DataPointer _dataPointerAtLoopStart;
    int _numStepsAtLastLoopExit;
    int _previousLoopLength;

    int _numLoopExits;

    void checkGliderContract();

public:
    GliderHangDetector(ExhaustiveSearcher& searcher);

    HangType hangType() { return HangType::APERIODIC_GLIDER; }

    void start();

    void signalLoopStartDetected();
    void signalLoopIterationCompleted();
    void signalLoopExit();

    HangDetectionResult detectHang();
};

#endif /* GliderHangDetector_h */
