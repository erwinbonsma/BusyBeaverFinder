//
//  PeriodicSweepHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 06/12/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//
#pragma once

#include "SweepHangDetector.h"

class PeriodicSweepTransitionGroup : public SweepTransitionGroup {
    friend class PeriodicSweepHangDetector;

    // The transitions, ordered as they occur during one period of the sweep.
    std::vector<const SweepTransition*> _transitions;

    bool hangIsMetaPeriodic() override { return true; }
    bool determineSweepEndType() override;
    bool onlyZeroesAhead(DataPointer dp, const Data& data) const override;

    void addTransition(const SweepTransition *transition);
    bool finishTransitionAnalysis();

    bool determineZeroExitSweepEndType() override;

public:
    // The first transition for this group. If the group has multiple transitions, the first
    // transition depends on when analysis is invoked, so is somewhat arbitrary. However, it also
    // means that it is the first transition that will be executed next, which therefore is useful
    // in analysing the data when trying to proof the hang.
    const SweepTransition* firstTransition() const { return _transitions[0]; };

    void clear() override;

    std::ostream& dump(std::ostream &os) const override;
};

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
class PeriodicSweepHangDetector : public SweepHangDetector {

    // The number of meta-loop iterations that comprise a fully-repeating sweep loop. A requirement
    // for a fully-repeating sweep loop is that the number of execution steps increases linearly
    // each iteration. The sweep repetition period is typically one, but can be larger when the
    // sweep has multiple fixed turning points at a given end of the sweep.
    int _sweepRepetitionPeriod;

    bool checkLinearIncrease(int start1, int start2, int start3) const;
    bool analyzeSweepIterations();

protected:
    bool shouldCheckNow(bool loopContinues) const override;

    bool analyzeHangBehaviour() override;
    bool analyzeTransitions() override;

    bool scanSweepSequence(DataPointer &dp, int fromEndIndex) override;

public:
    PeriodicSweepHangDetector(const ExecutionState& execution);

    virtual HangType hangType() const override { return HangType::REGULAR_SWEEP; }
};
