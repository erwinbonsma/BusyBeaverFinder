//
//  SweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 17/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "SweepHangDetector.h"

#include <iostream>
#include "Utils.h"

const int MAX_ITERATIONS_TRANSITION_LOOPS = 3;
const int MIN_ITERATIONS_LAST_SWEEP_LOOP = 5;

//#define SWEEP_DEBUG_TRACE

int numFailed = 0;
bool sweepHangFailure(const ProgramExecutor& executor) {
//    executor.dumpExecutionState();
    numFailed++;
    return false;
}

const SweepTransition* sweepTransitionScanFailure(const ProgramExecutor& executor) {
    numFailed++;
    return nullptr;
}

SweepTransitionScanner::SweepTransitionScanner(const SweepHangDetector &sweepHangDetector) :
    _sweepHangDetector(sweepHangDetector),
    _runSummary(sweepHangDetector._executor.getRunSummary()),
    _metaRunSummary(sweepHangDetector._executor.getMetaRunSummary())
{
    _nextLoopIndex = _runSummary.getNumRunBlocks() - 1;
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
    auto &executor = _sweepHangDetector._executor;
    int j = (_numSweeps + 1) % 2;

    if (group[j]->outgoingLoop() != group[1 - j]->incomingLoop()) {
        // Before the transition there is an extra outgoing loop that needs to be checked

        int prevLoopIndex = _sweepHangDetector.findPreviousSweepLoop(_nextLoopIndex);
        if (prevLoopIndex < 0) {
            return sweepTransitionScanFailure(executor);
        }

        if (_nextLoopIndex - prevLoopIndex > 1) {
            // There's a transition sequence separating both loops
            if (group[j]->midSweepTransition() == nullptr) {
                assert(_numUniqueTransitions < MAX_SWEEP_TRANSITION_ANALYSIS);
                SweepTransitionAnalysis *sta = &(_sweepHangDetector
                                                 ._transitionAnalysisPool[_numUniqueTransitions++]);

                if (!sta->analyzeSweepTransition(prevLoopIndex + 1, _nextLoopIndex, executor)) {
                    return sweepTransitionScanFailure(executor);
                }
                group[j]->setMidSweepTransition(sta);
            } else {
                if (!group[j]->midSweepTransition()->transitionEquals(prevLoopIndex + 1,
                                                                      _nextLoopIndex,
                                                                      executor)) {
                    return sweepTransitionScanFailure(executor);
                }
            }
        }
        _nextLoopIndex = prevLoopIndex;

        int ignored;
        if (!_sweepHangDetector.loopsAreEquivalent(_runSummary.runBlockAt(_nextLoopIndex),
                                                   group[j]->outgoingLoop()->loopRunBlock(),
                                                   ignored)
        ) {
            // Sequence does not follow expected sweep pattern anymore
            return sweepTransitionScanFailure(executor);
        }
    }

    int transitionStartIndex = findPrecedingTransitionStart(_nextLoopIndex);
    if (transitionStartIndex <= 0) {
        // This can happen when the sequence (and resulting sweeps) are still too short
        return sweepTransitionScanFailure(executor);
    }

    int loopIndex = transitionStartIndex - 1;
    const RunBlock *loopBlock = _runSummary.runBlockAt(loopIndex);
    int rotationEquivalenceOffset = 0;

    if (!_sweepHangDetector.loopsAreEquivalent(group[j]->incomingLoop()->loopRunBlock(), loopBlock,
                                               rotationEquivalenceOffset)
    ) {
        // Sequence does not follow expected sweep pattern anymore
        return sweepTransitionScanFailure(executor);
    }

    int loopLen = _runSummary.getRunBlockLength(loopIndex);
    int exitIndex = (loopLen - 1 + rotationEquivalenceOffset) % loopBlock->getLoopPeriod();
    const SweepTransition* st = group[j]->findTransitionMatching(exitIndex,
                                                                transitionStartIndex,
                                                                _nextLoopIndex,
                                                                executor);
    if (st == nullptr) {
        // This is a new transition. Add it
        assert(_numUniqueTransitions < MAX_SWEEP_TRANSITION_ANALYSIS);
        SweepTransitionAnalysis *sta = &(_sweepHangDetector.
                                         _transitionAnalysisPool[_numUniqueTransitions++]);

        if (!sta->analyzeSweepTransition(transitionStartIndex, _nextLoopIndex, executor)) {
            return sweepTransitionScanFailure(executor);
        }

        SweepTransition newTransition(sta, _nextLoopStartInstructionIndex);
        st = group[j]->addTransitionForExit(exitIndex, newTransition);
    } else {
        st->numOccurences += 1;
    }

    _nextLoopIndex = loopIndex;
    _nextLoopStartInstructionIndex = (rotationEquivalenceOffset != 0
                                      ? loopBlock->getLoopPeriod() - rotationEquivalenceOffset
                                      : 0);
    _numSweeps++;

    return st;
}

SweepHangDetector::SweepHangDetector(const ProgramExecutor& executor)
: HangDetector(executor) {}

SweepHangDetector::~SweepHangDetector() {
    for (int i = 0; i < 2; i++) {
        delete _transitionGroups[i];
        _transitionGroups[i] = nullptr;
    }
}

int SweepHangDetector::findPreviousSweepLoop(int runBlockIndex) const {
    const RunSummary& runSummary = _executor.getRunSummary();

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

bool SweepHangDetector::analyzeLoops() {
    const RunSummary& runSummary = _executor.getRunSummary();
    auto *group = _transitionGroups;
    int numSweepLoops = 0;
    int numAnalyzedLoops = 0;
    int runBlockIndex = runSummary.getNumRunBlocks();

    if (runSummary.getLoopIteration() < MIN_ITERATIONS_LAST_SWEEP_LOOP) {
        return sweepHangFailure(_executor);
    }

    while (numAnalyzedLoops < 4) {
        runBlockIndex = findPreviousSweepLoop(runBlockIndex);
        if (runBlockIndex < 0) {
            return false;
        }
        const RunBlock *runBlock = runSummary.runBlockAt(runBlockIndex);
        if (!_loopAnalysisPool[numSweepLoops++].analyzeSweepLoop(runBlock, _executor)) {
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
        return sweepHangFailure(_executor);
    }

    return true;
}

bool SweepHangDetector::loopsAreEquivalent(const RunBlock* loop1, const RunBlock* loop2,
                                           int &rotationEquivalenceOffset) const {
    assert(loop1->isLoop());
    assert(loop2->isLoop());

    return (
        loop1->getSequenceIndex() == loop2->getSequenceIndex() ||
        _executor.getRunSummary().areLoopsRotationEqual(loop1, loop2, rotationEquivalenceOffset)
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

bool SweepHangDetector::scanSweepSequenceAfterDelta(DataPointer &dp,
                                                    bool atRight, int initialDpDelta
) {
    const Data& data = _executor.getData();

    // For now, scan all values as the values that are skipped now may be expected during a next
    // sweep.
    // TODO?: Make analysis smarter.
    int delta = atRight ? -1 : 1;

    DataPointer dpEnd = (delta > 0) ? data.getMaxDataP() : data.getMinDataP();

    // DP is at one side of the sweep. Find the other end of the sweep.
    auto sweepGroup = _transitionGroups[0];
    auto sweepLoop = sweepGroup->outgoingLoop();
    bool sweepMakesPersistentChange = (sweepGroup->combinedSweepValueChangeType()
                                       != SweepValueChangeType::NO_CHANGE);
    dp += delta * initialDpDelta;
    while (*dp) {
        if (sweepLoop->isExitValue(*dp)) {
            // Found end of sweep

            if (sweepLoop != _transitionGroups[1]->incomingLoop()) {
                // This is a mid-sweep transition, continue with incoming sweep loop
                if (sweepGroup->midSweepTransition() != nullptr) {
                    dp += sweepGroup->midSweepTransition()->dataPointerDelta();
                }

                sweepGroup = _transitionGroups[1];
                sweepLoop = sweepGroup->incomingLoop();
                sweepMakesPersistentChange = (sweepGroup->combinedSweepValueChangeType()
                                              != SweepValueChangeType::NO_CHANGE);
            } else {
                break;
            }
        }

        if (sweepMakesPersistentChange && sweepGroup->canSweepChangeValueTowardsZero(*dp)) {
            // The sweep makes changes to the sequence that move some values towards zero
            return sweepHangFailure(_executor);
        }

        assert(dp != dpEnd); // Assumes abs(delta) == 1

        dp += delta;
    }

    return true;
}

bool SweepHangDetector::scanSweepSequence(DataPointer &dp, bool atRight) {
    return scanSweepSequenceAfterDelta(dp, atRight, 1);
}

void SweepHangDetector::clear() {
    for (SweepTransitionGroup *tg : _transitionGroups) {
        tg->clear();
    }
}

bool SweepHangDetector::analyzeHangBehaviour() {
    const RunSummary& runSummary = _executor.getRunSummary();

    if (runSummary.getNumRunBlocks() <= 8) {
        // The run should contain two full sweeps preceded by a loop: L0 (T0 L1 T1 L0) (T0 L1 T1 L0)
        // Note, transitions are named after the loop that precedes it (as they depend on the exit
        // of that loop).
        return false;
    }

    clear();

    if (!analyzeLoops()) {
        return false;
    }

//    std::cout
//    << "0-IN=" << _transitionGroups[0].incomingLoop()->loopRunBlock()->getSequenceIndex()
//    << ", 0-OUT=" << _transitionGroups[0].outgoingLoop()->loopRunBlock()->getSequenceIndex()
//    << ", 1-IN=" << _transitionGroups[1].incomingLoop()->loopRunBlock()->getSequenceIndex()
//    << ", 1-OUT=" << _transitionGroups[1].outgoingLoop()->loopRunBlock()->getSequenceIndex()
//    << std::endl;

//    _executor.dumpExecutionState();

    if (!analyzeTransitions()) {
        return false;
    }

//    ((const ExhaustiveSearcher &)_executor).getProgram().dumpWeb();

    if (!analyzeTransitionGroups()) {
        return sweepHangFailure(_executor);
    }

//    _executor.getInterpretedProgram().dump();
//    _executor.dumpExecutionState();
//    dump();

    return true;
}

Trilian SweepHangDetector::proofHang() {
    const Data& data = _executor.getData();
    DataPointer dp0 = data.getDataPointer();

//    data.dump();

    DataPointer dp1 = dp0; // Initial value

    if (!scanSweepSequence(dp1, _transitionGroups[0]->locatedAtRight())) {
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

//    _executor.getRunSummary().dump();
//    _executor.getMetaRunSummary().dump();
//    dump();
//    _executor.getData().dump();
//    _executor.getInterpretedProgram().dump();

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
        os << "Transition Group " << i << std::endl;
        os << *(detector._transitionGroups[i]) << std::endl;
    }

    return os;
}
