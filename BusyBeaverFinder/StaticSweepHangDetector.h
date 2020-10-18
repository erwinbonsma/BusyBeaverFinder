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
public:
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

    // Dynamic checks
    bool onlyZeroesAhead(bool atRight, bool skipNonZeroes);

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
