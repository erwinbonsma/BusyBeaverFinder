//
//  GliderHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef GliderHangDetector_h
#define GliderHangDetector_h

#include "HangDetector.h"

#include "LoopAnalysis.h"

const int maxAheadOffset = 16;

class GliderHangDetector : public HangDetector {

    SequenceAnalysis _transitionSequence;
    LoopAnalysis _loop;
    const RunBlock* _loopRunBlock;
    int _aheadDelta[maxAheadOffset];

    int _curCounterDpOffset, _curCounterDelta;
    int _nxtCounterDpOffset, _nxtCounterDelta;
    int _numBootstrapCycles;

    // How many iterations of the meta-loop before the next counter (the one whose absolute value
    // is being increased) becomes the current counter (the one whose absolute value is decreased).
    // It is typically one, but can be bigger for more complex glider loops.
    int _counterQueueSize;

    // Loop analysis
    bool identifyLoopCounter();
    bool isGliderLoop();
    bool analyzeLoop();

    // Sequence analysis
    bool determineCounterQueueSize();
    bool checkTransitionDeltas();
    bool analyzeTransitionSequence();

    // Dynamic checks
    bool isBootstrapping();
    bool transitionSequenceIsFixed();
    bool onlyZeroesAhead();

protected:
    bool shouldCheckNow(bool loopContinues) const override;

    bool analyzeHangBehaviour() override;

    Trilian proofHang() override;

public:
    GliderHangDetector(const ProgramExecutor& executor);

    virtual HangType hangType() const override { return HangType::APERIODIC_GLIDER; }
};

#endif /* GliderHangDetector_h */
