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

bool SweepHangDetector::determinePossibleSweepExitValues() {
    assert(_possibleSweepExitValues.empty());

    auto loop0 = _transitionGroups[0].incomingLoop(), loop1 = _transitionGroups[1].incomingLoop();
    for (int exitValue : loop1->exitValues() ) {
        _possibleSweepExitValues.insert(exitValue);
    }
    for (int exitValue : loop0->exitValues() ) {
        if (loop1->sweepValueChangeType() == SweepValueChangeType::UNIFORM_CHANGE ||
            loop1->sweepValueChangeType() == SweepValueChangeType::NO_CHANGE
        ) {
            _possibleSweepExitValues.insert(exitValue - loop1->sweepValueChange());
        } else {
            // TODO: Take into account all possible changes made by loop
            return false;
        }
    }

    assert(_sweepTransitionValues.empty());
    if (_transitionGroups[0].outgoingLoop() != _transitionGroups[1].incomingLoop()) {
        for (int exitValue : _transitionGroups[0].outgoingLoop()->exitValues()) {
            _sweepTransitionValues.insert(exitValue);
        }
    }

    return true;
}

bool SweepHangDetector::analyzeLoops() {
    const RunSummary& runSummary = _executor.getRunSummary();
    auto *group = _transitionGroups;
    int numSweepLoops = 0;
    int runBlockIndex = runSummary.getNumRunBlocks() - 1;
    const RunBlock *runBlock;

    // Assume that the loop which just finished is one of the sweep loops. If the sweep consists
    // of two loops, it should be the second loop (i.e. the incoming loop for the next transition)
    if (runSummary.getLoopIteration() < MIN_ITERATIONS_LAST_SWEEP_LOOP) {
        return sweepHangFailure(_executor);
    }

//    _executor.dumpExecutionState();

    runBlock = runSummary.runBlockAt(runBlockIndex);
    if (!_loopAnalysisPool[numSweepLoops++].analyzeSweepLoop(runBlock, _executor)) {
        return false;
    }
    group[0].setIncomingLoop(&_loopAnalysisPool[0]);

    // Check if there's a loop directly preceding it.
    --runBlockIndex;
    runBlock = runSummary.runBlockAt(runBlockIndex);
    if (runBlock->isLoop()) {
        // There is a loop. Check if it's part of the same sweep, the sweep in the other direction
        // or the transition that seperates them.
        int len = runSummary.getRunBlockLength(runBlockIndex);
        int numIterations = len / runBlock->getLoopPeriod();
        if (numIterations > MAX_ITERATIONS_TRANSITION_LOOPS) {
            // The loop is too big to be part of the transition. Analyze it
            if (!_loopAnalysisPool[numSweepLoops++].analyzeSweepLoop(runBlock, _executor)) {
                return false;
            }

            // Check its direction
            if (_loopAnalysisPool[0].movesRightwards() == _loopAnalysisPool[1].movesRightwards()) {
                // Both loops move in the same direction, the unusual case. This sweep apparently
                // consists of two different loops
                group[1].setOutgoingLoop(&_loopAnalysisPool[1]);
            } else {
                // It's apparently the incoming loop in the other direction (without a transition
                // in between)
                group[1].setIncomingLoop(&_loopAnalysisPool[1]);
            }
        }
    }

    if (group[1].outgoingLoop() == nullptr) {
        group[1].setOutgoingLoop(group[0].incomingLoop());
    }

    if (group[1].incomingLoop() == nullptr) {
        // Skip the transition (if any)
        runBlockIndex = findPrecedingTransitionStart(runBlockIndex);
        if (runBlockIndex == 0) {
            // There is no sweep loop preceding the transition
            return sweepHangFailure(_executor);
        }

        // The next loop must be the incoming loop at the other end of the sweep
        --runBlockIndex;
        runBlock = runSummary.runBlockAt(runBlockIndex);
        if (!_loopAnalysisPool[numSweepLoops].analyzeSweepLoop(runBlock, _executor)) {
            return sweepHangFailure(_executor);
        }
        group[1].setIncomingLoop(&_loopAnalysisPool[numSweepLoops++]);
    }

    // Check if there's a loop directly preceding it.
    --runBlockIndex;
    runBlock = runSummary.runBlockAt(runBlockIndex);
    if (runBlock->isLoop()) {
        // There is a loop. Check if it's part of the same sweep, the sweep in the other direction
        // or the transition that seperates them.
        int len = runSummary.getRunBlockLength(runBlockIndex);
        int numIterations = len / runBlock->getLoopPeriod();
        if (numIterations > MAX_ITERATIONS_TRANSITION_LOOPS) {
            // The loop is too big to be part of the transition. Analyze it
            if (!_loopAnalysisPool[numSweepLoops].analyzeSweepLoop(runBlock, _executor)) {
                return sweepHangFailure(_executor);
            }

            // Check its direction
            if (_loopAnalysisPool[numSweepLoops].movesRightwards() ==
                _loopAnalysisPool[numSweepLoops - 1].movesRightwards()
            ) {
                // Both loops move in the same direction, the unusual case. This sweep apparently
                // consists of two different loops
                group[0].setOutgoingLoop(&_loopAnalysisPool[numSweepLoops++]);
            }
        }
    }

    if (group[0].outgoingLoop() == nullptr) {
        group[0].setOutgoingLoop(group[1].incomingLoop());
    }

    if (!group[0].analyzeSweeps() || !group[1].analyzeSweeps()) {
        return false;
    }

    // Both loops should move in opposite directions
    if (group[0].locatedAtRight() == group[1].locatedAtRight()) {
        return sweepHangFailure(_executor);
    }

    if (!determinePossibleSweepExitValues()) {
        return false;
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
    const RunBlock *metaLoop = metaRunSummary.getLastRunBlock();
    auto *group = _transitionGroups;
    int metaLoop2Index = runSummary.getNumRunBlocks() - metaLoop->getLoopPeriod() - 1;
    int metaLoop1Index = metaLoop2Index - metaLoop->getLoopPeriod();
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
                // This can happen when the meta-loop comprises more than one full sweep.
                // TODO: Support optional transition from an outgoing loop to incoming loop that
                // sweep in the same direction.
                break;
            }

            int ignored;
            if (!loopsAreEquivalent(runSummary.runBlockAt(nextLoopIndex),
                                    group[j].outgoingLoop()->loopRunBlock(), ignored)
            ) {
                // Sequence does not follow expected sweep pattern anymore
                break;
            }
        }

        int transitionStartIndex = findPrecedingTransitionStart(nextLoopIndex);
        if (transitionStartIndex <= metaLoop1Index) {
            // This can happen when the sequence (and resulting sweeps) are still too short
            break;
        }

        int loopIndex = transitionStartIndex - 1;
        const RunBlock *loopBlock = runSummary.runBlockAt(loopIndex);
        int rotationEquivalenceOffset = 0;

        if (!loopsAreEquivalent(loopBlock, group[j].incomingLoop()->loopRunBlock(),
                                rotationEquivalenceOffset)
        ) {
            // Sequence does not follow expected sweep pattern anymore
            break;
        }

        int loopLen = runSummary.getRunBlockLength(loopIndex);
        int exitIndex = (loopLen - 1 + rotationEquivalenceOffset) % loopBlock->getLoopPeriod();
        const SweepTransition* st = group[j].transitionForExit(exitIndex);
        if (st != nullptr) {
            // Check that the transition is identical
            if (!st->transition->transitionEquals(transitionStartIndex, nextLoopIndex, _executor)) {
                // Transition does not match
                break;
            }

            group[j].setFirstTransition(*st);
        } else {
            // This is the first transition that follows the given loop exit
            assert(numUniqueTransitions < MAX_SWEEP_TRANSITION_ANALYSIS);
            SweepTransitionAnalysis *sta = &_transitionAnalysisPool[numUniqueTransitions++];

            if (!sta->analyzeSweepTransition(transitionStartIndex, nextLoopIndex, _executor)) {
                return sweepHangFailure(_executor);
            }

            SweepTransition newTransition(sta, nextLoopStartInstructionIndex);
            group[j].addTransitionForExit(exitIndex, newTransition);

            group[j].setFirstTransition(newTransition);
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
    auto groups = _transitionGroups;
    bool checkSweepTransition = groups[0].outgoingLoop() != groups[1].incomingLoop();
    auto sweepGroup = _transitionGroups[0];
    bool sweepMakesPersistentChange = (sweepGroup.combinedSweepValueChangeType()
                                       != SweepValueChangeType::NO_CHANGE);
    dp += delta;
    while (*dp) {
        if (checkSweepTransition &&
            _sweepTransitionValues.find(*dp) != _sweepTransitionValues.end()
        ) {
            sweepGroup = _transitionGroups[1];
            sweepMakesPersistentChange = (sweepGroup.combinedSweepValueChangeType()
                                          != SweepValueChangeType::NO_CHANGE);
            checkSweepTransition = false;
        }

        if (_possibleSweepExitValues.find(*dp) != _possibleSweepExitValues.end()) {
            // Found end of sweep at other end
            break;
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
    return !loopContinues && _executor.getMetaRunSummary().isInsideLoop();
}

void SweepHangDetector::clear() {
    for (SweepTransitionGroup &tg : _transitionGroups) {
        tg.clear();
    }
    _possibleSweepExitValues.clear();
    _sweepTransitionValues.clear();
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
