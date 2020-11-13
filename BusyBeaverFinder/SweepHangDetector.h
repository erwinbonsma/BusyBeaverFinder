//
//  SweepHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 17/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef SweepHangDetector_h
#define SweepHangDetector_h

#include "HangDetector.h"
#include "SweepTransitionGroup.h"

const int MAX_SWEEP_LOOP_ANALYSIS = 8;
const int MAX_SWEEP_TRANSITION_ANALYSIS = 8;

class SweepHangDetector : public HangDetector {
    friend SweepTransitionGroup;
    friend std::ostream &operator<<(std::ostream&, const SweepHangDetector&);

    SweepLoopAnalysis _loopAnalysisPool[MAX_SWEEP_LOOP_ANALYSIS];
    SweepTransitionAnalysis _transitionAnalysisPool[MAX_SWEEP_TRANSITION_ANALYSIS];
    SweepTransitionGroup _transitionGroups[2];

    SweepValueChangeType _sweepValueChangeType;
    int _sweepValueChange;

    std::set<int> _possibleSweepExitValues;

    /* Analysis */
    // Returns run block index of the transition that precedes the given sweep loop. If there is
    // no transition between subsequent sweep loops, it returns sweepLoopRunBlockIndex.
    int findPrecedingTransitionStart(int sweepLoopRunBlockIndex) const;

    int numTransitions() const {
        return _transitionGroups[0].numTransitions() + _transitionGroups[1].numTransitions();
    }

    // The combined change to the sequence made by both the sweep-loops
    bool determineCombinedSweepValueChange();
    SweepValueChangeType combinedSweepValueChangeType() const { return _sweepValueChangeType; }
    int combinedSweepValueChange() const { return _sweepValueChange; }

    // If only one of the sweeps makes a uniform change and the other loop makes no change, returns
    // the value of this change. Returns 0 otherwise
    int singleSweepValueChange() const;

    bool determinePossibleSweepExitValues();

    bool canSweepChangeValueTowardsZero(int value) const;

    bool analyzeLoops();
    bool analyzeTransitions();
    bool analyzeTransitionGroups();

    /* Dynamic checks */
    // Finds the point where the appendix connects to the sweep body
    DataPointer findAppendixStart(DataPointer dp, const SweepTransitionGroup &group);
    // Find the other end of the sequence. Updates dp accordingly. When deltaSign is non-zero, it
    // verifies that this delta moves the entire sequence away from zero.
    bool scanSweepSequence(DataPointer &dp, bool atRight);

protected:
    bool shouldCheckNow(bool loopContinues);

    bool analyzeHangBehaviour();

    Trilian proofHang();

public:
    SweepHangDetector(const ProgramExecutor& executor);

    virtual HangType hangType() const { return HangType::REGULAR_SWEEP; }

    const SweepTransitionGroup& transitionGroup(bool atRight) const;

    void dump() const;
};

std::ostream &operator<<(std::ostream &os, const SweepHangDetector &detector);

#endif /* SweepHangDetector_h */
