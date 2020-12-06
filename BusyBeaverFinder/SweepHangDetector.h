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

const int MAX_SWEEP_TRANSITION_ANALYSIS = 16;
const int MAX_SWEEP_LOOP_ANALYSIS = 4;

class SweepHangDetector;

class SweepTransitionScanner {
    const SweepHangDetector &_sweepHangDetector;
    const RunSummary &_runSummary;
    const RunSummary &_metaRunSummary;

    int _nextLoopIndex;
    int _nextLoopStartInstructionIndex;
    int _numSweeps;
    int _numUniqueTransitions;

    // Returns run block index of the transition that precedes the given sweep loop. If there is
    // no transition between subsequent sweep loops, it returns sweepLoopRunBlockIndex.
    int findPrecedingTransitionStart(int sweepLoopRunBlockIndex) const;

public:
    SweepTransitionScanner(const SweepHangDetector &sweepHangDetector);

    int nextLoopIndex() const { return _nextLoopIndex; }
    int numSweeps() const { return _numSweeps; }

    // Go back in the run history to find and analyze the previous sweep transition. If no
    // transition can be found because the run history does not exhibit a sweep behavior (anymore),
    // returns nullptr.
    const SweepTransition* analyzePreviousSweepTransition();
};

class SweepHangDetector : public HangDetector {
    friend std::ostream &operator<<(std::ostream&, const SweepHangDetector&);
    friend SweepTransitionScanner;

protected:
    mutable SweepTransitionAnalysis _transitionAnalysisPool[MAX_SWEEP_TRANSITION_ANALYSIS];
    SweepLoopAnalysis _loopAnalysisPool[MAX_SWEEP_LOOP_ANALYSIS];
    SweepTransitionGroup *_transitionGroups[2];

    /* Analysis */
    int findPreviousSweepLoop(int runBlockIndex) const;

    bool loopsAreEquivalent(const RunBlock* loop1, const RunBlock *loop2,
                            int &rotationEquivalenceOffset) const;

    bool analyzeLoops();
    virtual bool analyzeTransitions() = 0;
    bool analyzeTransitionGroups();

    /* Dynamic checks */
    // Find the other end of the sequence. Updates dp accordingly. When deltaSign is non-zero, it
    // verifies that this delta moves the entire sequence away from zer*o.
    bool scanSweepSequenceAfterDelta(DataPointer &dp, bool atRight, int initialDpDelta);
    virtual bool scanSweepSequence(DataPointer &dp, bool atRight);

    bool analyzeHangBehaviour();

    Trilian proofHang();

    void clear();

public:
    SweepHangDetector(const ProgramExecutor& executor);
    ~SweepHangDetector();

    const SweepTransitionGroup& transitionGroup(bool atRight) const;

    void dump() const;
};

std::ostream &operator<<(std::ostream &os, const SweepHangDetector &detector);

// Helper functions for testing. These rely on the last detected hang being a sweep hang
SweepEndType rightSweepEndType(const ProgressTracker &tracker);
SweepEndType leftSweepEndType(const ProgressTracker &tracker);

#endif /* SweepHangDetector_h */
