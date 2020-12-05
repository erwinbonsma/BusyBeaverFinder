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
 *   It therefore does not detect loops with a FIXED_APERIODIC_APPENDIX sweep end-type, which
 *   execute a kind of binary count
 * - A sweep in a given direction always executes the same loop (under rotation-equivalence)
 *   Note: The rotation-equivalence allowance means that the entry instruction for this loop may
 *   differ
 *   - The loop may switch mid-sweep to another one. In that case, the outgoing loop should always
 *     be the same (under rotation-equivalence), and the incoming loop should always be the same.
 */
class SweepHangDetector : public HangDetector {
    friend SweepTransitionGroup;
    friend std::ostream &operator<<(std::ostream&, const SweepHangDetector&);

    SweepTransitionAnalysis _transitionAnalysisPool[MAX_SWEEP_TRANSITION_ANALYSIS];
    SweepLoopAnalysis _loopAnalysisPool[MAX_SWEEP_LOOP_ANALYSIS];
    PeriodicSweepTransitionGroup _transitionGroups[2];

    // The number of meta-loop iterations that comparise a fully-repeating sweep loop. A requirement
    // for a fully-repeating sweep loop is that the number of execution steps increases linearly
    // each iteration. The sweep repetition period is typically one, but can be larger when the
    // sweep has multiple fixed turning points at a given end of the sweep.
    int _sweepRepetitionPeriod;

    /* Analysis */
    // Returns run block index of the transition that precedes the given sweep loop. If there is
    // no transition between subsequent sweep loops, it returns sweepLoopRunBlockIndex.
    int findPrecedingTransitionStart(int sweepLoopRunBlockIndex) const;

    int findPreviousSweepLoop(int runBlockIndex) const;

    bool loopsAreEquivalent(const RunBlock* loop1, const RunBlock *loop2,
                            int &rotationEquivalenceOffset) const;

    bool analyzeSweepIterations();
    bool analyzeLoops();
    bool analyzeTransitions();
    bool analyzeTransitionGroups();

    /* Dynamic checks */
    // Find the other end of the sequence. Updates dp accordingly. When deltaSign is non-zero, it
    // verifies that this delta moves the entire sequence away from zero.
    bool scanSweepSequence(DataPointer &dp, bool atRight);

protected:
    bool shouldCheckNow(bool loopContinues);

    bool analyzeHangBehaviour();

    Trilian proofHang();

    void clear();

public:
    SweepHangDetector(const ProgramExecutor& executor);

    virtual HangType hangType() const { return HangType::REGULAR_SWEEP; }

    const SweepTransitionGroup& transitionGroup(bool atRight) const;

    void dump() const;
};

std::ostream &operator<<(std::ostream &os, const SweepHangDetector &detector);

#endif /* SweepHangDetector_h */
