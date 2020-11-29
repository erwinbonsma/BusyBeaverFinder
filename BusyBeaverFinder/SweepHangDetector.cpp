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

SweepHangDetector::SweepHangDetector(const ProgramExecutor& executor)
: HangDetector(executor) {}

bool SweepHangDetector::analyzeSweepIterations() {
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

    return sweepHangFailure(_executor);
}

int SweepHangDetector::findPrecedingTransitionStart(int sweepLoopRunBlockIndex) const {
    const RunSummary& runSummary = _executor.getRunSummary();
    int startIndex = sweepLoopRunBlockIndex;

    while (startIndex > 0) {
        int nextIndex = startIndex - 1;
        const RunBlock* runBlock = runSummary.runBlockAt(nextIndex);
        if (runBlock->isLoop()) {
            int iterations = runSummary.getRunBlockLength(nextIndex) / runBlock->getLoopPeriod();

            if (iterations > MAX_ITERATIONS_TRANSITION_LOOPS) {
                // This loop is too big to be included in the transition
                break;
            }
        }

        startIndex = nextIndex;
    }

    return startIndex;
}

// If both sweeps only make a single change, returns that. Otherwise returns 0.
int SweepHangDetector::singleSweepValueChange() const {
    auto loop0 = _transitionGroups[0].incomingLoop(), loop1 =_transitionGroups[0].outgoingLoop();
    auto type0 = loop0->sweepValueChangeType(), type1 = loop1->sweepValueChangeType();

    if (type0 == SweepValueChangeType::NO_CHANGE && type1 == SweepValueChangeType::UNIFORM_CHANGE) {
        return loop1->sweepValueChange();
    }

    if (type1 == SweepValueChangeType::NO_CHANGE && type0 == SweepValueChangeType::UNIFORM_CHANGE) {
        return loop0->sweepValueChange();
    }

    return 0;
}

int SweepHangDetector::findPreviousSweepLoop(int runBlockIndex) const {
    const RunSummary& runSummary = _executor.getRunSummary();

    while (--runBlockIndex >= 0) {
        const RunBlock *runBlock = runSummary.runBlockAt(runBlockIndex);

        if (runBlock->isLoop()) {
            int len = runSummary.getRunBlockLength(runBlockIndex);
            int numIterations = len / runBlock->getLoopPeriod();
            if (numIterations > MAX_ITERATIONS_TRANSITION_LOOPS) {
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
                group[0].setIncomingLoop(&_loopAnalysisPool[0]);
                numAnalyzedLoops += 1;
                break;
            case 1:
                if (_loopAnalysisPool[0].movesRightwards() ==
                    _loopAnalysisPool[1].movesRightwards()
                ) {
                    // This sweep apparently consists of two different loops
                    group[1].setOutgoingLoop(&_loopAnalysisPool[1]);
                    numAnalyzedLoops += 1;
                } else {
                    // It's apparently the incoming loop in the other direction
                    group[1].setOutgoingLoop(&_loopAnalysisPool[0]);
                    group[1].setIncomingLoop(&_loopAnalysisPool[1]);
                    numAnalyzedLoops += 2;
                }
                break;
            case 2:
                group[1].setIncomingLoop(&_loopAnalysisPool[numSweepLoops - 1]);
                numAnalyzedLoops += 1;
                break;
            case 3:
                if (_loopAnalysisPool[numSweepLoops - 2].movesRightwards() ==
                    _loopAnalysisPool[numSweepLoops - 1].movesRightwards()
                ) {
                    // This sweep apparently consists of two different loops
                    group[0].setOutgoingLoop(&_loopAnalysisPool[numSweepLoops - 1]);
                    numAnalyzedLoops += 1;
                } else {
                    // It's apparently the incoming loop in the other direction
                    group[0].setOutgoingLoop(&_loopAnalysisPool[numSweepLoops - 2]);
                    numSweepLoops -= 1;
                    numAnalyzedLoops += 1;
                }
        }
    }

    if (!group[0].analyzeSweeps() || !group[1].analyzeSweeps()) {
        return false;
    }

    // Both loops should move in opposite directions
    if (group[0].locatedAtRight() == group[1].locatedAtRight()) {
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

bool SweepHangDetector::analyzeTransitions() {
    const RunSummary &runSummary = _executor.getRunSummary();
    const RunSummary &metaRunSummary = _executor.getMetaRunSummary();
    auto *group = _transitionGroups;

    const RunBlock *metaLoop = metaRunSummary.getLastRunBlock();
    int sweepLoopPeriod = metaLoop->getLoopPeriod() * _sweepRepetitionPeriod;

    int metaLoop2Index = runSummary.getNumRunBlocks() - sweepLoopPeriod - 1;
    int metaLoop1Index = metaLoop2Index - sweepLoopPeriod;
    int nextLoopIndex = runSummary.getNumRunBlocks() - 1;
    int nextLoopStartInstructionIndex = 0;
    int numSweeps = 0, numUniqueTransitions = 0;

    // Check two iterations of the meta-loop, starting at the last. This is done to check that any
    // loops inside transition sequences run for a fixed number of iterations.
    while (nextLoopIndex > metaLoop1Index) {
        int j = (numSweeps + 1) % 2;

        if (group[j].outgoingLoop() != group[1 - j].incomingLoop()) {
            // Before the transition there is an extra outgoing loop that needs to be checked

            --nextLoopIndex;
            if (!runSummary.runBlockAt(nextLoopIndex)->isLoop()) {
                if (group[j].midSweepTransition() == nullptr) {
                    assert(numUniqueTransitions < MAX_SWEEP_TRANSITION_ANALYSIS);
                    SweepTransitionAnalysis *sta = &_transitionAnalysisPool[numUniqueTransitions++];

                    if (!sta->analyzeSweepTransition(nextLoopIndex, nextLoopIndex + 1, _executor)) {
                        return sweepHangFailure(_executor);
                    }
                    group[j].setMidSweepTransition(sta);
                }
                --nextLoopIndex;
            }

            int ignored;
            if (!loopsAreEquivalent(runSummary.runBlockAt(nextLoopIndex),
                                    group[j].outgoingLoop()->loopRunBlock(), ignored)
            ) {
                // Sequence does not follow expected sweep pattern anymore
                return sweepHangFailure(_executor);
            }
        }

        int transitionStartIndex = findPrecedingTransitionStart(nextLoopIndex);
        if (transitionStartIndex <= metaLoop1Index) {
            // This can happen when the sequence (and resulting sweeps) are still too short
            return sweepHangFailure(_executor);
        }

        int loopIndex = transitionStartIndex - 1;
        const RunBlock *loopBlock = runSummary.runBlockAt(loopIndex);
        int rotationEquivalenceOffset = 0;

        if (!loopsAreEquivalent(group[j].incomingLoop()->loopRunBlock(), loopBlock,
                                rotationEquivalenceOffset)
        ) {
            // Sequence does not follow expected sweep pattern anymore
            return sweepHangFailure(_executor);
        }

        int loopLen = runSummary.getRunBlockLength(loopIndex);
        int exitIndex = (loopLen - 1 + rotationEquivalenceOffset) % loopBlock->getLoopPeriod();
        const SweepTransition* st = group[j].findTransitionMatching(exitIndex,
                                                                    transitionStartIndex,
                                                                    nextLoopIndex,
                                                                    _executor);
        if (st == nullptr) {
            if (nextLoopIndex <= metaLoop2Index) {
                // No new transition should be encountered after one iteration of the meta-loop
                return sweepHangFailure(_executor);
            }

            // This is a new transition. Add it
            assert(numUniqueTransitions < MAX_SWEEP_TRANSITION_ANALYSIS);
            SweepTransitionAnalysis *sta = &_transitionAnalysisPool[numUniqueTransitions++];

            if (!sta->analyzeSweepTransition(transitionStartIndex, nextLoopIndex, _executor)) {
                return sweepHangFailure(_executor);
            }

            SweepTransition newTransition(sta, nextLoopStartInstructionIndex);
            group[j].addTransitionForExit(exitIndex, newTransition);

            group[j].setFirstTransition(newTransition);
        } else {
            group[j].setFirstTransition(*st);
        }

        nextLoopIndex = loopIndex;
        nextLoopStartInstructionIndex = (rotationEquivalenceOffset != 0
                                         ? loopBlock->getLoopPeriod() - rotationEquivalenceOffset
                                         : 0);
        numSweeps++;
    }

    if (nextLoopIndex != metaLoop1Index) {
        // For a regular sweep (locked into a meta-loop), the entire iteration of the meta-loop
        // should be analysed and match the expected sweep behavior.
        return sweepHangFailure(_executor);
    }

    if (numSweeps % 2 != 0) {
        // Should never happen. If so, should investigate
        assert(false);

        return sweepHangFailure(_executor);
    }

    return true;
}

bool SweepHangDetector::analyzeTransitionGroups() {
    for (SweepTransitionGroup &tg : _transitionGroups) {
        if (!tg.analyzeGroup()) {
            return false;
        }
    }

    return true;
}

DataPointer SweepHangDetector::findAppendixStart(DataPointer dp,
                                                 const SweepTransitionGroup &group) {
    const Data& data = _executor.getData();
    int delta = group.locatedAtRight() ? -1 : 1;

    while (true) {
        int val = data.valueAt(dp, delta);
        if (val == 0 || !group.incomingLoop()->isExitValue(val)) {
            break;
        }
        dp += delta;
    }

    return dp;
}

bool SweepHangDetector::scanSweepSequence(DataPointer &dp, bool atRight) {
    const Data& data = _executor.getData();

    // For now, scan all values as the values that are skipped now may be expected during a next
    // sweep.
    // TODO?: Make analysis smarter.
    int delta = atRight ? -1 : 1;

    DataPointer dpEnd = (delta > 0) ? data.getMaxDataP() : data.getMinDataP();

    // DP is at one side of the sweep. Find the other end of the sweep.
    auto sweepGroup = _transitionGroups[0];
    auto sweepLoop = sweepGroup.outgoingLoop();
    bool sweepMakesPersistentChange = (sweepGroup.combinedSweepValueChangeType()
                                       != SweepValueChangeType::NO_CHANGE);
    dp += delta * std::max(1, abs(sweepGroup.firstTransition().transition->dataPointerDelta()));
    while (*dp) {
        if (sweepLoop->isExitValue(*dp)) {
            // Found end of sweep

            if (sweepLoop != _transitionGroups[1].incomingLoop()) {
                // This is a mid-sweep transition, continue with incoming sweep loop
                if (sweepGroup.midSweepTransition() != nullptr) {
                    dp += sweepGroup.midSweepTransition()->dataPointerDelta();
                }

                sweepGroup = _transitionGroups[1];
                sweepLoop = sweepGroup.incomingLoop();
                sweepMakesPersistentChange = (sweepGroup.combinedSweepValueChangeType()
                                              != SweepValueChangeType::NO_CHANGE);
            } else {
                break;
            }
        }

        if (sweepMakesPersistentChange && sweepGroup.canSweepChangeValueTowardsZero(*dp)) {
            // The sweep makes changes to the sequence that move some values towards zero
            return sweepHangFailure(_executor);
        }

        assert(dp != dpEnd); // Assumes abs(dataPointerDelta) == 1

        dp += delta;
    }

    return true;
}

bool SweepHangDetector::shouldCheckNow(bool loopContinues) {
    // Should wait for the sweep-loop to finish
    return (!loopContinues &&
            _executor.getMetaRunSummary().isInsideLoop() &&
            _executor.getMetaRunSummary().getLoopIteration() >= 3);
}

void SweepHangDetector::clear() {
    for (SweepTransitionGroup &tg : _transitionGroups) {
        tg.clear();
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

    //((const ExhaustiveSearcher &)_executor).getProgram().dumpWeb();

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
//    if (_transitionGroups[1].endType() == SweepEndType::FIXED_GROWING) {
//        dp0 = findAppendixStart(dp0, _transitionGroups[1]);
//    }

    if (!scanSweepSequence(dp1, _transitionGroups[0].locatedAtRight())) {
        return Trilian::MAYBE;
    }
    assert(dp0 != dp1);

    for (int i = 0; i < 2; ++i) {
        DataPointer dp = (i == 0) ? dp0 : dp1;
        Trilian result = _transitionGroups[i].proofHang(dp, data);

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
    return _transitionGroups[(int) (_transitionGroups[1].locatedAtRight() == atRight)];
}

void SweepHangDetector::dump() const {
    std::cout << *this << std::endl;
}

std::ostream &operator<<(std::ostream &os, const SweepHangDetector &detector) {
    for (int i = 0; i < 2; i++) {
        os << "Transition Group " << i << std::endl;
        os << detector._transitionGroups[i] << std::endl;
    }

    return os;
}
