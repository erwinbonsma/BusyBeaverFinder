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
#include <vector>
#include <map>

const int MAX_UNIQUE_TRANSITIONS_PER_SWEEP = 8;

class SweepLoopAnalysis : public LoopAnalysis {
    friend std::ostream &operator<<(std::ostream&, const SweepLoopAnalysis&);

    // If the loop makes any changes, this gives the sign of the delta. For now it is assumed that
    // the sign of all deltas match.
    int _deltaSign;

    std::vector<int> _exitValues;

public:
    int deltaSign() const { return _deltaSign; }
    bool isExitValue(int value);

    bool analyseSweepLoop(RunBlock* runBlock, ExhaustiveSearcher& searcher);
};

std::ostream &operator<<(std::ostream &os, const SweepLoopAnalysis& sta);

class SweepTransitionAnalysis : public SequenceAnalysis {
    bool _extendsSweep;
public:
    bool extendsSweep() const { return _extendsSweep; }

    bool analyseSweepTransition(RunBlock* runBlock, bool atRight, ExhaustiveSearcher& searcher);

    void dump() const;
};

std::ostream &operator<<(std::ostream &os, const SweepTransitionAnalysis& sta);

// TODO: Extend to also include loop that precedes it?
struct SweepTransitionGroup {
    std::map<int, SweepTransitionAnalysis*> transitions;
    bool positionIsFixed;

    void clear();
};

class StaticSweepHangDetector : public StaticHangDetector {
    friend std::ostream &operator<<(std::ostream&, const StaticSweepHangDetector&);

    SweepLoopAnalysis _loop[2];
    RunBlock* _loopRunBlock[2];

    SweepTransitionAnalysis _transitionPool[MAX_UNIQUE_TRANSITIONS_PER_SWEEP];
    SweepTransitionGroup _transitionGroup[2];

    int _sweepDeltaSign;

    // Loop analysis
    bool analyseLoops();

    // Sequence analysis
    bool analyseTransitions();

    /* Dynamic checks */
    // Find the other end of the sequence. Updates dp accordingly. When deltaSign is non-zero, it
    // verifies that this delta moves the entire sequence away from zero.
    bool scanSweepSequence(DataPointer &dp, SweepLoopAnalysis &sweepLoop);
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
