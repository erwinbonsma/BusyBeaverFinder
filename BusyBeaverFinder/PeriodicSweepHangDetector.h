//
//  PeriodicSweepHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 06/12/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef PeriodicSweepHangDetector_h
#define PeriodicSweepHangDetector_h

#include "SweepHangDetector.h"

class PeriodicSweepTransitionGroup : public SweepTransitionGroup {
    SweepTransition _firstTransition;

    bool determineSweepEndType() override;
    bool onlyZeroesAhead(DataPointer dp, const Data& data) const override;

public:
    // The first transition for this group. If the group has multiple transitions, the first
    // transition depends on when analysis is invoked, so is somewhat arbitrary. However, it also
    // means that it is the first transition that will be executed next, which therefore is useful
    // in analysing the data when trying to proof the hang.
    SweepTransition firstTransition() const { return _firstTransition; };

    void setFirstTransition(SweepTransition transition) { _firstTransition = transition; }

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

    // The number of meta-loop iterations that comparise a fully-repeating sweep loop. A requirement
    // for a fully-repeating sweep loop is that the number of execution steps increases linearly
    // each iteration. The sweep repetition period is typically one, but can be larger when the
    // sweep has multiple fixed turning points at a given end of the sweep.
    int _sweepRepetitionPeriod;

    bool analyzeSweepIterations();

protected:
    bool shouldCheckNow(bool loopContinues) override;

    bool analyzeHangBehaviour() override;
    bool analyzeTransitions() override;

    bool scanSweepSequence(DataPointer &dp, bool atRight) override;

public:
    PeriodicSweepHangDetector(const ProgramExecutor& executor);
};

#endif /* PeriodicSweepHangDetector_h */
