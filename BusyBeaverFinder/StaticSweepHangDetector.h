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

    bool analyzeSweepLoop(RunBlock* runBlock, ExhaustiveSearcher& searcher);
};

std::ostream &operator<<(std::ostream &os, const SweepLoopAnalysis& sta);

class SweepTransitionAnalysis : public SequenceAnalysis {
    bool _extendsSweep;
public:
    bool extendsSweep() const { return _extendsSweep; }

    bool analyzeSweepTransition(RunBlock* runBlock, bool atRight, ExhaustiveSearcher& searcher);

    void dump() const;
};

std::ostream &operator<<(std::ostream &os, const SweepTransitionAnalysis& sta);

class SweepTransitionGroup {
    friend std::ostream &operator<<(std::ostream&, const SweepTransitionGroup&);

    SweepLoopAnalysis _loop;
    RunBlock* _loopRunBlock;

    std::map<int, SweepTransitionAnalysis*> _transitions;
    bool _positionIsFixed;
    bool _locatedAtRight;
    int _sweepDeltaSign;

    DataDeltas _outsideDeltas;

public:
    bool locatedAtRight() const { return _locatedAtRight; }
    bool positionIsFixed() const { return _positionIsFixed; }

    // The direction (away from zero) that the loop and/or transitions change values that are part
    // of the sweep.
    //
    // For now it is assumed that for hangs all deltas should have the same sign. More complex
    // hangs could violate this assumption.
    int sweepDeltaSign() const { return _sweepDeltaSign; }

    const DataDeltas& outsideDeltas() const { return _outsideDeltas; }

    SweepLoopAnalysis& loop() { return _loop; }
    RunBlock* loopRunBlock() { return _loopRunBlock; }

    bool hasTransitionForExit(int instructionIndex) {
        return _transitions.find(instructionIndex) != _transitions.end();
    }
    void addTransitionForExit(SweepTransitionAnalysis *sta, int instructionIndex) {
        _transitions[instructionIndex] = sta;
    }

    bool analyzeLoop(RunBlock* runBlock, ExhaustiveSearcher& searcher);
    bool analyzeGroup();
};

std::ostream &operator<<(std::ostream &os, const SweepTransitionGroup &group);

class StaticSweepHangDetector : public StaticHangDetector {
    friend std::ostream &operator<<(std::ostream&, const StaticSweepHangDetector&);

    SweepTransitionAnalysis _transitionPool[MAX_UNIQUE_TRANSITIONS_PER_SWEEP];
    SweepTransitionGroup _transitionGroups[2];

    int _sweepDeltaSign;

    // Analysis
    bool analyzeLoops();
    bool analyzeTransitions();
    bool analyzeTransitionGroups();

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
