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

const int MAX_SWEEP_TRANSITION_ANALYSIS = 8;
const int MAX_SWEEP_LOOP_ANALYSIS = 4;

/* Detector for sweep hangs.
 *
 * It should identify all sweep hangs for which the following holds:
 * - The hang results in an endless meta-run loop (i.e. the hang is regular)
 *   It therefore does not detect loops with a FIXED_GROWING sweep end-type, which execute a kind of
 *   binary count
 * - A sweep in a given direction always executes the same loop (under rotation-equivalence)
 *   Note: The rotation-equivalence allowance means that the entry instruction for this loop may
 *   differ
 */
class SweepHangDetector : public HangDetector {
    friend SweepTransitionGroup;
    friend std::ostream &operator<<(std::ostream&, const SweepHangDetector&);

    SweepTransitionAnalysis _transitionAnalysisPool[MAX_SWEEP_TRANSITION_ANALYSIS];
    SweepLoopAnalysis _loopAnalysisPool[MAX_SWEEP_LOOP_ANALYSIS];
    PeriodicSweepTransitionGroup _transitionGroups[2];

    std::set<int> _possibleSweepExitValues;

    /* Analysis */
    // Returns run block index of the transition that precedes the given sweep loop. If there is
    // no transition between subsequent sweep loops, it returns sweepLoopRunBlockIndex.
    int findPrecedingTransitionStart(int sweepLoopRunBlockIndex) const;

    int numTransitions() const {
        return _transitionGroups[0].numTransitions() + _transitionGroups[1].numTransitions();
    }

    // If only one of the sweeps makes a uniform change and the other loop makes no change, returns
    // the value of this change. Returns 0 otherwise
    int singleSweepValueChange() const;

    bool determinePossibleSweepExitValues();

    bool loopsAreEquivalent(const RunBlock* loop1, const RunBlock *loop2,
                            int &rotationEquivalenceOffset) const;

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
