//
//  SweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 17/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "SweepHangDetector.h"

#include <iostream>

#include "HangExecutor.h"
#include "RunSummary.h"
#include "Utils.h"

const int MAX_ITERATIONS_TRANSITION_LOOPS = 3;
const int MIN_ITERATIONS_LAST_SWEEP_LOOP = 5;

//#define SWEEP_DEBUG_TRACE

int numFailed = 0;
bool sweepHangFailure(const ExecutionState& execution) {
//    execution.dumpExecutionState();
    numFailed++;
    return false;
}

const SweepTransition* sweepTransitionScanFailure(const ExecutionState& execution) {
    numFailed++;
    return nullptr;
}

SweepTransitionScanner::SweepTransitionScanner(const SweepHangDetector &sweepHangDetector) :
    _sweepHangDetector(sweepHangDetector),
    _runSummary(sweepHangDetector._execution.getRunSummary()),
    _metaRunSummary(sweepHangDetector._execution.getMetaRunSummary())
{
    _nextLoopIndex = _runSummary.getNumRunBlocks() - 1;
    _nextLoopStartInstructionIndex = 0;
    _numSweeps = 0;
    _numUniqueTransitions = (
        (sweepHangDetector._transitionGroups[0]->midSweepTransition() != nullptr ? 1 : 0) +
        (sweepHangDetector._transitionGroups[1]->midSweepTransition() != nullptr ? 1 : 0)
    );
}

int SweepTransitionScanner::findPrecedingTransitionStart(int sweepLoopRunBlockIndex) const {
    int startIndex = sweepLoopRunBlockIndex;

    while (startIndex > 0) {
        int nextIndex = startIndex - 1;
        const RunBlock* runBlock = _runSummary.runBlockAt(nextIndex);
        if (runBlock->isLoop()) {
            int len = _runSummary.getRunBlockLength(nextIndex);

            if (len > MAX_ITERATIONS_TRANSITION_LOOPS * runBlock->getLoopPeriod()) {
                // This loop is too big to be included in the transition
                break;
            }
        }

        startIndex = nextIndex;
    }

    return startIndex;
}

const SweepTransition* SweepTransitionScanner::analyzePreviousSweepTransition() {
    auto group = _sweepHangDetector._transitionGroups;
    auto &execution = _sweepHangDetector._execution;
    int j = (_numSweeps + 1) % 2;
    int lastLoopIndex = _nextLoopIndex;

    if (group[j]->outgoingLoop() != group[1 - j]->incomingLoop() ||
        group[j]->midSweepTransition() != nullptr
    ) {
        // The sweep consists of two parts, possibly separated by a mid-sweep transition

        int prevLoopIndex = _sweepHangDetector.findPreviousSweepLoop(_nextLoopIndex);
        if (prevLoopIndex < 0) {
            return sweepTransitionScanFailure(execution);
        }

        if (_nextLoopIndex - prevLoopIndex > 1) {
            // There's a transition sequence separating both loops
            if (group[j]->midSweepTransition() == nullptr) {
                return sweepTransitionScanFailure(execution);
            }
            if (!group[j]->midSweepTransition()->transitionEquals(prevLoopIndex + 1,
                                                                  _nextLoopIndex,
                                                                  execution)) {
                return sweepTransitionScanFailure(execution);
            }
        }
        _nextLoopIndex = prevLoopIndex;

        int ignored;
        if (!_sweepHangDetector.loopsAreEquivalent(_runSummary.runBlockAt(_nextLoopIndex),
                                                   group[j]->outgoingLoop()->loopRunBlock(),
                                                   ignored)
        ) {
            // Sequence does not follow expected sweep pattern anymore
            return sweepTransitionScanFailure(execution);
        }
    }

    int transitionStartIndex = findPrecedingTransitionStart(_nextLoopIndex);
    if (transitionStartIndex <= 0) {
        // This can happen when the sequence (and resulting sweeps) are still too short
        return sweepTransitionScanFailure(execution);
    }

    int loopIndex = transitionStartIndex - 1;
    const RunBlock *loopBlock = _runSummary.runBlockAt(loopIndex);
    int rotationEquivalenceOffset = 0;

    if (!_sweepHangDetector.loopsAreEquivalent(group[j]->incomingLoop()->loopRunBlock(), loopBlock,
                                               rotationEquivalenceOffset)
    ) {
        // Sequence does not follow expected sweep pattern anymore
        return sweepTransitionScanFailure(execution);
    }

    int loopLen = _runSummary.getRunBlockLength(loopIndex);
    int exitIndex = (loopLen - 1 + rotationEquivalenceOffset) % loopBlock->getLoopPeriod();
    const SweepTransition* st = group[j]->findTransitionMatching(exitIndex,
                                                                transitionStartIndex,
                                                                _nextLoopIndex,
                                                                execution);
    if (st == nullptr) {
        // This is a new transition. Add it
        if (_numUniqueTransitions >= MAX_SWEEP_TRANSITION_ANALYSIS) {
            return sweepTransitionScanFailure(execution);
        }
        SweepTransitionAnalysis *sta = &(_sweepHangDetector.
                                         _transitionAnalysisPool[_numUniqueTransitions++]);

        if (!sta->analyzeSweepTransition(transitionStartIndex, _nextLoopIndex, execution)) {
            return sweepTransitionScanFailure(execution);
        }

        SweepTransition newTransition(sta, _nextLoopStartInstructionIndex);
        st = group[j]->addTransitionForExit(exitIndex, newTransition);
    } else {
        st->numOccurences += 1;
    }

    _nextLoopIndex = loopIndex;
    _nextLoopStartInstructionIndex = rotationEquivalenceOffset;
    _numSweeps++;

    _lastSweepLength = abs(getDpDelta(execution.getRunSummary(), execution.getInterpretedProgram(),
                                      transitionStartIndex, lastLoopIndex + 1));

    return st;
}

SweepHangDetector::SweepHangDetector(const ExecutionState& execution)
: HangDetector(execution) {}

SweepHangDetector::~SweepHangDetector() {
    for (int i = 0; i < 2; i++) {
        delete _transitionGroups[i];
        _transitionGroups[i] = nullptr;
    }
}

bool SweepHangDetector::shouldCheckNow(bool loopContinues) const {
    const RunSummary& runSummary = _execution.getRunSummary();

    // Should wait for the sweep-loop to finish
    return (
        !loopContinues &&

        // Analysis should latch onto a sweep-loop (not a fixed loop part of a transition sequence)
        runSummary.getLoopIteration() >= MIN_ITERATIONS_LAST_SWEEP_LOOP &&

        // The run should contain two full sweeps preceded by a loop: L0 (T0 L1 T1 L0) (T0 L1 T1 L0)
        // Note, transitions are named after the loop that precedes it (as they depend on the exit
        // of that loop).
        runSummary.getNumRunBlocks() > 8
    );
}

int SweepHangDetector::findPreviousSweepLoop(int runBlockIndex) const {
    const RunSummary& runSummary = _execution.getRunSummary();

    while (--runBlockIndex >= 0) {
        const RunBlock *runBlock = runSummary.runBlockAt(runBlockIndex);

        if (runBlock->isLoop()) {
            int len = runSummary.getRunBlockLength(runBlockIndex);
            if (len > MAX_ITERATIONS_TRANSITION_LOOPS * runBlock->getLoopPeriod()) {
                break;
            }
        }
    }

    return runBlockIndex;
}

void SweepHangDetector::populateExitDeltas() {
    // Determine sweep exit deltas
    auto it = _sweepLengths.crbegin(); // Use reverse iterator to add deltas in chronological order
    int prevLen = *it;
    int tgIndex = _sweepLengths.size() % 2;
    while (++it != _sweepLengths.crend()) {
        int delta = *it - prevLen;
        prevLen = *it;
        _transitionGroups[tgIndex]->addExitDelta(delta);
        tgIndex = 1 - tgIndex;
    }
}

bool SweepHangDetector::analyzeMidSweepTransitionIfAny(int runBlockIndexOutgoingLoop,
                                                       int runBlockIndexIncomingLoop,
                                                       bool isFirstSweep) {
    if (runBlockIndexOutgoingLoop + 1 == runBlockIndexIncomingLoop) {
        // There is no transition sequence separating both sweep loops
        return true;
    }

    SweepTransitionAnalysis *sta =
        &_transitionAnalysisPool[_transitionGroups[1]->midSweepTransition() ? 1 : 0];

    if (!sta->analyzeSweepTransition(runBlockIndexOutgoingLoop + 1,
                                     runBlockIndexIncomingLoop, _execution)) {
        return sweepTransitionScanFailure(_execution);
    }

    _transitionGroups[isFirstSweep ? 1 : 0]->setMidSweepTransition(sta);

    return true;
}

bool SweepHangDetector::analyzeLoops() {
    const RunSummary& runSummary = _execution.getRunSummary();
    auto *group = _transitionGroups;
    int numSweepLoops = 0;
    int numAnalyzedLoops = 0;
    int runBlockIndex = runSummary.getNumRunBlocks();

    while (numAnalyzedLoops < 4) {
        int nextIndex = runBlockIndex;
        runBlockIndex = findPreviousSweepLoop(runBlockIndex);
        if (runBlockIndex < 0) {
            return false;
        }
        const RunBlock *runBlock = runSummary.runBlockAt(runBlockIndex);
        if (!_loopAnalysisPool[numSweepLoops++].analyzeSweepLoop(runBlock, _execution)) {
            return false;
        }

        switch (numAnalyzedLoops) {
            case 0:
                group[0]->setIncomingLoop(&_loopAnalysisPool[0]);
                numAnalyzedLoops += 1;
                break;
            case 1:
                if (_loopAnalysisPool[0].movesRightwards() ==
                    _loopAnalysisPool[1].movesRightwards()
                ) {
                    // This sweep apparently consists of two different loops
                    group[1]->setOutgoingLoop(&_loopAnalysisPool[1]);
                    numAnalyzedLoops += 1;

                    if (!analyzeMidSweepTransitionIfAny(runBlockIndex, nextIndex, true)) {
                        return false;
                    }
                } else {
                    // It's apparently the incoming loop in the other direction
                    group[1]->setOutgoingLoop(&_loopAnalysisPool[0]);
                    group[1]->setIncomingLoop(&_loopAnalysisPool[1]);
                    numAnalyzedLoops += 2;
                }
                break;
            case 2:
                group[1]->setIncomingLoop(&_loopAnalysisPool[numSweepLoops - 1]);
                numAnalyzedLoops += 1;
                break;
            case 3:
                if (_loopAnalysisPool[numSweepLoops - 2].movesRightwards() ==
                    _loopAnalysisPool[numSweepLoops - 1].movesRightwards()
                ) {
                    // This sweep apparently consists of two different loops
                    group[0]->setOutgoingLoop(&_loopAnalysisPool[numSweepLoops - 1]);
                    numAnalyzedLoops += 1;

                    if (!analyzeMidSweepTransitionIfAny(runBlockIndex, nextIndex, false)) {
                        return false;
                    }
                } else {
                    // It's apparently the incoming loop in the other direction
                    group[0]->setOutgoingLoop(&_loopAnalysisPool[numSweepLoops - 2]);
                    numSweepLoops -= 1;
                    numAnalyzedLoops += 1;
                }
        }
    }

    if (!group[0]->analyzeSweeps() || !group[1]->analyzeSweeps()) {
        return false;
    }

    // Both loops should move in opposite directions
    if (group[0]->locatedAtRight() == group[1]->locatedAtRight()) {
        return sweepHangFailure(_execution);
    }

    return true;
}

bool SweepHangDetector::loopsAreEquivalent(const RunBlock* loop1, const RunBlock* loop2,
                                           int &rotationEquivalenceOffset) const {
    assert(loop1->isLoop());
    assert(loop2->isLoop());

    return (
        loop1->getSequenceIndex() == loop2->getSequenceIndex() ||
        _execution.getRunSummary().areLoopsRotationEqual(loop1, loop2, rotationEquivalenceOffset)
    );
}

bool SweepHangDetector::analyzeTransitionGroups() {
    for (SweepTransitionGroup* tg : _transitionGroups) {
        if (!tg->analyzeGroup()) {
            return false;
        }
    }

    return true;
}

bool SweepHangDetector::scanSweepSequenceAfterDelta(DataPointer &dp, int fromEndIndex,
                                                    int initialDpDelta
) {
    const Data& data = _execution.getData();

    // DP is at one side of the sweep. Find the other end of the sweep.
    auto sweepGroup = _transitionGroups[fromEndIndex];
    auto sweepLoop = sweepGroup->outgoingLoop();
    bool sweepMakesPersistentChange = (sweepGroup->combinedSweepValueChangeType()
                                       != SweepValueChangeType::NO_CHANGE);

    // For now, scan all values as the values that are skipped now may be expected during a next
    // sweep.
    // TODO?: Make analysis smarter.
    int delta = sweepGroup->locatedAtRight() ? -1 : 1;

    DataPointer dpEnd = (delta > 0) ? data.getMaxDataP() : data.getMinDataP();

    dp += delta * initialDpDelta;
    while (*dp) {
        if (sweepLoop->isExitValue(*dp)) {
            // Found end of sweep

            if (sweepLoop != _transitionGroups[1 - fromEndIndex]->incomingLoop()) {
                // This is a mid-sweep transition, continue with incoming sweep loop
                if (
                    sweepGroup->midSweepTransition() != nullptr &&
                    sign(sweepGroup->midSweepTransition()->dataPointerDelta()) == sign(delta)
                ) {
                    // Skip over any possible exit values
                    dp += sweepGroup->midSweepTransition()->dataPointerDelta();

                    // Note: If delta direction differs from sweep direction DP is not adjusted to
                    // ensure that DP changes only the expected direction to never violate the
                    // post-condition that DP should always differ from the initial value and
                    // shifted in the right direction. This should never impact analysis of actual
                    // sweep hangs.
                }

                sweepGroup = _transitionGroups[1 - fromEndIndex];
                sweepLoop = sweepGroup->incomingLoop();
                sweepMakesPersistentChange = (sweepGroup->combinedSweepValueChangeType()
                                              != SweepValueChangeType::NO_CHANGE);
            } else {
                break;
            }
        }

        if (sweepMakesPersistentChange && sweepGroup->canSweepChangeValueTowardsZero(*dp)) {
            // The sweep makes changes to the sequence that move some values towards zero
            return sweepHangFailure(_execution);
        }

        assert(dp != dpEnd); // Assumes abs(delta) == 1

        dp += delta;
    }

    return true;
}

bool SweepHangDetector::scanSweepSequence(DataPointer &dp, int fromEndIndex) {
    return scanSweepSequenceAfterDelta(dp, fromEndIndex, 1);
}

void SweepHangDetector::clearAnalysis() {
    HangDetector::clearAnalysis();

    for (SweepTransitionGroup *tg : _transitionGroups) {
        tg->clear();
    }

    _sweepLengths.clear();
}

bool SweepHangDetector::analyzeHangBehaviour() {
    if (oldAnalysisAvailable()) {
        clearAnalysis(); // TEMP
    }

    if (!analyzeLoops()) {
        return false;
    }

    if (!analyzeTransitions()) {
        return false;
    }

    if (!analyzeTransitionGroups()) {
        return sweepHangFailure(_execution);
    }

    return true;
}

Trilian SweepHangDetector::proofHang() {
    const Data& data = _execution.getData();
    DataPointer dp0 = data.getDataPointer();

    DataPointer dp1 = dp0; // Initial value

//    std::cout << *this << std::endl;
//    _execution.getData().dump();

    if (!scanSweepSequence(dp1, 0)) {
        return Trilian::MAYBE;
    }
    assert(dp0 != dp1);

    for (int i = 0; i < 2; ++i) {
        DataPointer dp = (i == 0) ? dp0 : dp1;
        Trilian result = _transitionGroups[i]->proofHang(dp, data);

        if (result != Trilian::YES) {
            return result;
        }
    }

    return Trilian::YES;
}

const SweepTransitionGroup& SweepHangDetector::transitionGroup(bool atRight) const {
    return *_transitionGroups[(int) (_transitionGroups[1]->locatedAtRight() == atRight)];
}

void SweepHangDetector::dump() const {
    std::cout << *this << std::endl;
}

std::ostream &operator<<(std::ostream &os, const SweepHangDetector &detector) {
    for (int i = 0; i < 2; i++) {
        auto tg = detector._transitionGroups[i];
        os << "Transition Group " << i << " @";
        os << (tg->locatedAtRight() ? "Right" : "Left") << std::endl;
        os << *tg << std::endl;
    }

    return os;
}

SweepEndType sweepEndType(const HangExecutor& executor, bool atRight) {
    return ((const SweepHangDetector *)executor.detectedHang()
            )->transitionGroup(atRight).endType();
}

SweepEndType rightSweepEndType(const HangExecutor& executor) {
    return sweepEndType(executor, true);
}

SweepEndType leftSweepEndType(const HangExecutor& executor) {
    return sweepEndType(executor, false);
}
