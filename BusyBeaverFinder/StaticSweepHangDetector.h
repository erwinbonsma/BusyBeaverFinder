//
//  StaticSweepHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 17/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef StaticSweepHangDetector_h
#define StaticSweepHangDetector_h

#include "StaticHangDetector.h"

#include "LoopAnalysis.h"

class SweepLoopAnalysis : public LoopAnalysis {
    // If the loop makes any changes, this gives the sign of the delta. For now it is assumed that
    // the sign of all deltas match.
    int _deltaSign;

public:
    int deltaSign() const { return _deltaSign; }

    bool analyseSweepLoop(RunBlock* runBlock, ExhaustiveSearcher& searcher);
};

class SweepTransitionAnalysis : public SequenceAnalysis {
    bool _extendsSweep;
public:
    bool extendsSweep() const { return _extendsSweep; }

    bool analyseSweepTransition(RunBlock* runBlock, bool atRight, ExhaustiveSearcher& searcher);

    void dump() const;
};

std::ostream &operator<<(std::ostream &os, const SweepTransitionAnalysis& sta);

class StaticSweepHangDetector : public StaticHangDetector {
    friend std::ostream &operator<<(std::ostream&, const StaticSweepHangDetector&);

    SweepLoopAnalysis _loop[2];
    RunBlock* _loopRunBlock[2];

    SweepTransitionAnalysis _transition[2];
    RunBlock* _transitionRunBlock[2];

    // Loop analysis
    bool analyseLoops();

    // Sequence analysis
    bool analyseTransitions();

    /* Dynamic checks */
    // Find the other end of the sequence. Updates dp accordingly. When deltaSign is non-zero, it
    // verifies that this delta moves the entire sequence away from zero.
    bool scanSweepSequence(DataPointer &dp, bool atRight, int deltaSign);
    bool onlyZeroesAhead(DataPointer &dp, bool atRight);

protected:
    bool shouldCheckNow(bool loopContinues);

    bool analyzeHangBehaviour();

    Trilian proofHang();

public:
    StaticSweepHangDetector(ExhaustiveSearcher& searcher);

    virtual HangType hangType() { return HangType::REGULAR_SWEEP; }

    void dump() const;
};

std::ostream &operator<<(std::ostream &os, const StaticSweepHangDetector &detector);

#endif /* StaticSweepHangDetector_h */
