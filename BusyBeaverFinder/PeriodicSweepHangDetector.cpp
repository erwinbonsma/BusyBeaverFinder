//
//  PeriodicSweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 06/12/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "PeriodicSweepHangDetector.h"

int numPeriodicSweepFailures = 0;
bool periodicSweepHangFailure(const ProgramExecutor& executor) {
//    executor.dumpExecutionState();
    numPeriodicSweepFailures++;
    return false;
}

bool periodicSweepHangFailure(const PeriodicSweepTransitionGroup& transitionGroup) {
    numPeriodicSweepFailures++;
    return false;
}

bool PeriodicSweepTransitionGroup::onlyZeroesAhead(DataPointer dp, const Data& data) const {
    int maxOvershoot = 0;
    for (auto kv : _firstTransition.transition->preConditions()) {
        int dpOffset = kv.first;
        if (dpOffset != 0 && locatedAtRight() == (dpOffset > 0)) {
            PreCondition preCondition = kv.second;

            if (!preCondition.holdsForValue(data.valueAt(dp, dpOffset))) {
                return periodicSweepHangFailure(*this);
            }

            maxOvershoot = std::max(abs(dpOffset), maxOvershoot);
        }
    }

    if (maxOvershoot > 0) {
        // The transition examines data cells external to the exit data cell. These do not have to
        // be zero. Some transition sequences result in values beyond the sweep end, which move
        // outwards as the sequence grows.
        if (locatedAtRight()) {
            if (data.getMaxBoundP() - maxOvershoot <= dp) {
                return true;
            } else {
                dp += maxOvershoot;
            }
        } else {
            if (data.getMinDataP() + maxOvershoot >= dp) {
                return true;
            } else {
                dp -= maxOvershoot;
            }
        }
    }

    if (!SweepTransitionGroup::onlyZeroesAhead(dp, data)) {
        return false;
    }

    return true;
}


PeriodicSweepHangDetector::PeriodicSweepHangDetector(const ProgramExecutor& executor)
: SweepHangDetector(executor) {
    for (int i = 0; i < 2; i++ ) {
        _transitionGroups[i] = new PeriodicSweepTransitionGroup();
    }
}

bool PeriodicSweepHangDetector::analyzeSweepIterations() {
    const RunSummary &runSummary = _executor.getRunSummary();
    const RunSummary &metaRunSummary = _executor.getMetaRunSummary();
    const RunBlock *metaLoop = metaRunSummary.getLastRunBlock();

    _sweepRepetitionPeriod = 1;
    while (_sweepRepetitionPeriod * 3 <= _executor.getMetaRunSummary().getLoopIteration()) {
        int sweepLoopPeriod = metaLoop->getLoopPeriod() * _sweepRepetitionPeriod;

        int startLoop3 = runSummary.getNumRunBlocks() - sweepLoopPeriod;
        int startLoop2 = startLoop3 - sweepLoopPeriod;
        int startLoop1 = startLoop2 - sweepLoopPeriod;

        int lenLoop1 = runSummary.getRunBlockLength(startLoop1, startLoop2);
        int lenLoop2 = runSummary.getRunBlockLength(startLoop2, startLoop3);
        int lenLoop3 = runSummary.getRunBlockLength(startLoop3, runSummary.getNumRunBlocks());

        if ((lenLoop2 - lenLoop1) == (lenLoop3 - lenLoop2)) {
            return true;
        }

        _sweepRepetitionPeriod += 1;
    }

    return periodicSweepHangFailure(_executor);
}

bool PeriodicSweepHangDetector::analyzeTransitions() {
    const RunSummary &runSummary = _executor.getRunSummary();
    const RunSummary &metaRunSummary = _executor.getMetaRunSummary();

    const RunBlock *metaLoop = metaRunSummary.getLastRunBlock();
    int sweepLoopPeriod = metaLoop->getLoopPeriod() * _sweepRepetitionPeriod;

    int metaLoop2Index = runSummary.getNumRunBlocks() - sweepLoopPeriod - 1;
    int metaLoop1Index = metaLoop2Index - sweepLoopPeriod;

    SweepTransitionScanner transitionScanner(*this);

    // Check two iterations of the meta-loop, starting at the last. This is done to check that any
    // loops inside transition sequences run for a fixed number of iterations.
    while (transitionScanner.nextLoopIndex() > metaLoop1Index) {
        const SweepTransition* st = transitionScanner.analyzePreviousSweepTransition();
        if (st == nullptr) {
            return false;
        }

        if (st->numOccurences == 1 && transitionScanner.nextLoopIndex() <= metaLoop2Index) {
            // No new transition should be encountered after one iteration of the meta-loop
            return periodicSweepHangFailure(_executor);
        }

        ((PeriodicSweepTransitionGroup *)_transitionGroups[transitionScanner.numSweeps() % 2]
         )->setFirstTransition(*st);
    }

    if (transitionScanner.nextLoopIndex() != metaLoop1Index) {
        // For a regular sweep (locked into a meta-loop), the entire iteration of the meta-loop
        // should be analysed and match the expected sweep behavior.
        return periodicSweepHangFailure(_executor);
    }

    if (transitionScanner.numSweeps() % 2 != 0) {
        // Should never happen. If so, should investigate
        assert(false);

        return periodicSweepHangFailure(_executor);
    }

    if (_transitionGroups[0]->hasUniqueTransitions() ||
        _transitionGroups[1]->hasUniqueTransitions()
    ) {
        // Should never happen. If so, should investigate
        assert(false);

        return periodicSweepHangFailure(_executor);
    }

    return true;
}

bool PeriodicSweepHangDetector::scanSweepSequence(DataPointer &dp, bool atRight) {
    auto sweepGroup = (PeriodicSweepTransitionGroup *)_transitionGroups[0];
    int initialDelta = std::max(1, abs(sweepGroup->firstTransition()
                                       .transition->dataPointerDelta()));

    return scanSweepSequenceAfterDelta(dp, atRight, initialDelta);
}

bool PeriodicSweepHangDetector::shouldCheckNow(bool loopContinues) {
    // Should wait for the sweep-loop to finish
    return (!loopContinues &&
            _executor.getMetaRunSummary().isInsideLoop() &&
            _executor.getMetaRunSummary().getLoopIteration() >= 3);
}

bool PeriodicSweepHangDetector::analyzeHangBehaviour() {
    if (!analyzeSweepIterations()) {
        return false;
    }

    return SweepHangDetector::analyzeHangBehaviour();
}
