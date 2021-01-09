//
//  PeriodicSweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 06/12/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "PeriodicSweepHangDetector.h"

#include <algorithm>

int numPeriodicSweepFailures = 0;
bool periodicSweepHangFailure(const ProgramExecutor& executor) {
//    executor.dumpExecutionState();
//    executor.getData().dump();
    numPeriodicSweepFailures++;
    return false;
}

bool periodicSweepHangFailure(const PeriodicSweepTransitionGroup& transitionGroup) {
    numPeriodicSweepFailures++;
    return false;
}

bool PeriodicSweepTransitionGroup::determineSweepEndType() {
    if (!SweepTransitionGroup::determineSweepEndType()) {
        return false;
    }

    if (endType() == SweepEndType::FIXED_APERIODIC_APPENDIX) {
        // Unsupported for periodic sweeps
        return periodicSweepHangFailure(*this);
    }

    return true;
}

void PeriodicSweepTransitionGroup::addTransition(const SweepTransition *transition) {
    _transitions.push_back(transition);
}

bool PeriodicSweepTransitionGroup::finishTransitionAnalysis() {
    if (hasUniqueTransitions()) {
        // As two iterations of the meta-loop have been scanned. Each transition should occur
        // (at least) twice.
        return periodicSweepHangFailure(*this);
    }

    // Order transitions chronologically
    std::reverse(std::begin(_transitions), std::end(_transitions));

    return true;
}

bool PeriodicSweepTransitionGroup::onlyZeroesAhead(DataPointer dp, const Data& data) const {
    // TODO: Make smarter
    // It should fully exploit the periodic analysis and from that determine how many values
    // external to DP (a loop exit) have already been visited by the sweep which do not need
    // be zero (and also what value they should be for the sweep hang to continue as indefinitely).

    int maxOvershoot = 0;
    for (auto kv : _transitions[0]->transition->preConditions()) {
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

void PeriodicSweepTransitionGroup::clear() {
    SweepTransitionGroup::clear();

    _transitions.clear();
    _sweepExitDeltas.clear();
}

bool PeriodicSweepTransitionGroup::isSweepGrowing() const {
    int sum = 0;

    for (int delta : _sweepExitDeltas) {
        sum += delta;
    }

    return sum > 0;
}

bool PeriodicSweepTransitionGroup::isSweepGrowthConstant() const {
    auto it = _sweepExitDeltas.cbegin();
    int firstDelta = *it;

    while (++it != _sweepExitDeltas.cend()) {
        if (*it != firstDelta) {
            return false;
        }
    }

    return true;
}

std::ostream& PeriodicSweepTransitionGroup::dump(std::ostream &os) const {
    SweepTransitionGroup::dump(os);
    os << std::endl;

    os << "Transitions:" << std::endl;
    for (auto tr : _transitions) {
        os << *(tr->transition) << std::endl;
    }

    os << "Sweep exit deltas:";
    for (int delta : _sweepExitDeltas) {
        os << " " << delta;
    }
    os << std::endl;

    return os;
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

std::vector<int> tmpSweepLengths;
bool PeriodicSweepHangDetector::analyzeTransitions() {
    const RunSummary &runSummary = _executor.getRunSummary();
    const RunSummary &metaRunSummary = _executor.getMetaRunSummary();

    const RunBlock *metaLoop = metaRunSummary.getLastRunBlock();
    int sweepLoopPeriod = metaLoop->getLoopPeriod() * _sweepRepetitionPeriod;

    auto sweepLengths = tmpSweepLengths;
    sweepLengths.clear();

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

        if (transitionScanner.nextLoopIndex() >= metaLoop2Index) {
            // First iteration of meta-loop
            ((PeriodicSweepTransitionGroup *)_transitionGroups[transitionScanner.numSweeps() % 2]
             )->addTransition(st);

            sweepLengths.push_back(transitionScanner.lastSweepLength());
        } else {
            if (st->numOccurences == 1) {
                // No new transition should be encountered after one iteration of the meta-loop
                return periodicSweepHangFailure(_executor);
            }

            if (sweepLengths.size() % 2 == 0) {
                // Add 2*N+1 sweep lengths to be able to calculate 2*N deltas
                sweepLengths.push_back(transitionScanner.lastSweepLength());
            }
        }
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

    for (auto tg : _transitionGroups) {
        if (! ((PeriodicSweepTransitionGroup *)tg)->finishTransitionAnalysis()) {
            return periodicSweepHangFailure(_executor);
        }
    }

    // Determine sweep exit deltas
    auto it = sweepLengths.crbegin(); // Use reverse iterator to add deltas in chronological order
    int prevLen = *it;
    int numSweeps = 0;
    while (++it != sweepLengths.crend()) {
        int delta = *it - prevLen;
        prevLen = *it;
        ++numSweeps;
        ((PeriodicSweepTransitionGroup *)_transitionGroups[numSweeps % 2])->addExitDelta(delta);
    }

    return true;
}

bool PeriodicSweepHangDetector::scanSweepSequence(DataPointer &dp, int fromEndIndex) {
    auto sweepGroup = (PeriodicSweepTransitionGroup *)_transitionGroups[fromEndIndex];
    int initialDelta = abs(sweepGroup->firstTransition()->transition->dataPointerDelta()) + 1;

    return scanSweepSequenceAfterDelta(dp, fromEndIndex, initialDelta);
}

bool PeriodicSweepHangDetector::shouldCheckNow(bool loopContinues) const {
    // Should wait for the sweep-loop to finish
    return (SweepHangDetector::shouldCheckNow(loopContinues) &&
            _executor.getMetaRunSummary().isInsideLoop() &&
            _executor.getMetaRunSummary().getLoopIteration() >= 3);
}

bool PeriodicSweepHangDetector::analyzeHangBehaviour() {
    if (!analyzeSweepIterations()) {
        return false;
    }

    if (!SweepHangDetector::analyzeHangBehaviour()) {
        return false;
    }

    return true;
}
