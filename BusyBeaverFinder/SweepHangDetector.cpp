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
: HangDetector(executor) {
    _transitionGroups[0].init(this, &_transitionGroups[1]);
    _transitionGroups[1].init(this, &_transitionGroups[0]);
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

bool SweepHangDetector::determineCombinedSweepValueChange() {
    auto loop0 = _transitionGroups[0].loop(), loop1 =_transitionGroups[1].loop();
    auto type0 = loop0.sweepValueChangeType(), type1 = loop1.sweepValueChangeType();

    if (type0 == SweepValueChangeType::NO_CHANGE) {
        _sweepValueChangeType = type1;
        _sweepValueChange = loop1.sweepValueChange();
    } else if (type1 == SweepValueChangeType::NO_CHANGE) {
        _sweepValueChangeType = type0;
        _sweepValueChange = loop0.sweepValueChange();
    } else if (
        type0 == SweepValueChangeType::UNIFORM_CHANGE &&
        type1 == SweepValueChangeType::UNIFORM_CHANGE
    ) {
        _sweepValueChange = loop0.sweepValueChange() + loop1.sweepValueChange();
        _sweepValueChangeType = (_sweepValueChange
                                 ? SweepValueChangeType::UNIFORM_CHANGE
                                 : SweepValueChangeType::NO_CHANGE);
    } else if (
        type0 != SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES &&
        type1 != SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES &&
        sign(loop0.sweepValueChange()) == sign(loop1.sweepValueChange())
    ) {
        _sweepValueChangeType = SweepValueChangeType::MULTIPLE_ALIGNED_CHANGES;
        // Only the sign matters, so addition is not needed, but makes it nicely symmetrical
        _sweepValueChange = loop0.sweepValueChange() + loop1.sweepValueChange();
    } else {
        // Both loops make opposite changes that do not fully cancel out. We cannot (yet?) detect
        // hangs of this type.
        return sweepHangFailure(_executor);
    }

    if (
        (loop0.requiresFixedInput() || loop1.requiresFixedInput()) &&
        _sweepValueChangeType != SweepValueChangeType::NO_CHANGE
    ) {
        // The changes of both loops, if any, should cancel each other out. They don't
        return sweepHangFailure(_executor);
    }

    return true;
}

// If both sweeps only make a single change, returns that. Otherwise returns 0.
int SweepHangDetector::singleSweepValueChange() const {
    auto loop0 = _transitionGroups[0].loop(), loop1 =_transitionGroups[1].loop();
    auto type0 = loop0.sweepValueChangeType(), type1 = loop1.sweepValueChangeType();

    if (type0 == SweepValueChangeType::NO_CHANGE && type1 == SweepValueChangeType::UNIFORM_CHANGE) {
        return loop1.sweepValueChange();
    }

    if (type1 == SweepValueChangeType::NO_CHANGE && type0 == SweepValueChangeType::UNIFORM_CHANGE) {
        return loop0.sweepValueChange();
    }

    return 0;
}

bool SweepHangDetector::determinePossibleSweepExitValues() {
    _possibleSweepExitValues.clear();

    auto loop0 = _transitionGroups[0].loop(), loop1 = _transitionGroups[1].loop();
    for (int exitValue : loop0.exitValues() ) {
        _possibleSweepExitValues.insert(exitValue);
    }
    for (int exitValue : loop1.exitValues() ) {
        if (loop0.sweepValueChangeType() == SweepValueChangeType::UNIFORM_CHANGE ||
            loop0.sweepValueChangeType() == SweepValueChangeType::NO_CHANGE
        ) {
            _possibleSweepExitValues.insert(exitValue - loop0.sweepValueChange());
        } else {
            // TODO: Take into account all possible changes made by loop
            return false;
        }
    }

    return true;
}

bool SweepHangDetector::canSweepChangeValueTowardsZero(int value) const {
    if (value == 0 ||
        _sweepValueChangeType == SweepValueChangeType::NO_CHANGE
    ) {
        return false;
    }

    if (_transitionGroups[0].loop().canSweepChangeValueTowardsZero(value) ||
        _transitionGroups[1].loop().canSweepChangeValueTowardsZero(value)
    ) {
        return true;
    }

    return false;
}

bool SweepHangDetector::analyzeLoops() {
    const RunSummary& runSummary = _executor.getRunSummary();
    SweepTransitionGroup *group = _transitionGroups;

    // Assume that the loop which just finished is one of the sweep loops
    if (runSummary.getLoopIteration() < MIN_ITERATIONS_LAST_SWEEP_LOOP) {
        return sweepHangFailure(_executor);
    }
    int runBlockIndexLoop1 = runSummary.getNumRunBlocks() - 1;
    if (!group[1].analyzeLoop(runBlockIndexLoop1, _executor)) {
        return false;
    }

    int runBlockIndexTransition0 = findPrecedingTransitionStart(runBlockIndexLoop1);
    if (runBlockIndexTransition0 == 0) {
        // There is no sweep loop preceding the transition
        return sweepHangFailure(_executor);
    }
    int runBlockIndexLoop0 = runBlockIndexTransition0 - 1;
    if (!group[0].analyzeLoop(runBlockIndexLoop0, _executor)) {
        return false;
    }

    // Both loops should move in opposite directions
    if (group[0].locatedAtRight() == group[1].locatedAtRight()) {
        return sweepHangFailure(_executor);
    }

    if (!determineCombinedSweepValueChange()) {
        return false;
    }

    if (!determinePossibleSweepExitValues()) {
        return false;
    }

    return true;
}

bool SweepHangDetector::analyzeTransitions() {
    const RunSummary& runSummary = _executor.getRunSummary();
    const RunSummary& metaRunSummary = _executor.getMetaRunSummary();
    int metaLoopStartIndex = metaRunSummary.getLastRunBlock()->getStartIndex();
    int prevLoopIndex = runSummary.getNumRunBlocks() - 1;
    int prevLoopStartIndex = 0;
    int numSweeps = 0, numUniqueTransitions = 0;

    while (prevLoopIndex > metaLoopStartIndex) {
        int transitionStartIndex = findPrecedingTransitionStart(prevLoopIndex);
        if (transitionStartIndex <= metaLoopStartIndex) {
            // No more run blocks remain
            break;
        }

        int loopIndex = transitionStartIndex - 1;
        const RunBlock* loopBlock = runSummary.runBlockAt(loopIndex);
        assert(loopBlock->isLoop());

        int j = numSweeps % 2;
        SweepTransitionGroup &tg = _transitionGroups[j];
        const RunBlock* existingLoopBlock = tg.loop().loopRunBlock();
        int rotationEquivalenceOffset = 0;
        if (loopBlock->getSequenceIndex() != existingLoopBlock->getSequenceIndex() &&
            !runSummary.areLoopsRotationEqual(loopBlock, existingLoopBlock,
                                              rotationEquivalenceOffset)
        ) {
            // Sequence does not follow expected sweep pattern anymore
            break;
        }

        int loopLen = runSummary.getRunBlockLength(loopIndex);
        int exitIndex = (loopLen - 1 + rotationEquivalenceOffset) % loopBlock->getLoopPeriod();
        const SweepTransition* st = tg.transitionForExit(exitIndex);
        if (st != nullptr) {
            // Check that the transition is identical
            if (!st->transition->transitionEquals(transitionStartIndex, prevLoopIndex, _executor)) {
                // Transition does not match. This may be due to start-up effects. This could still
                // be a hang.
                break;
            }
        } else {
            // This is the first transition that follows the given loop exit
            assert(numUniqueTransitions < MAX_SWEEP_TRANSITION_ANALYSIS);
            SweepTransitionAnalysis *sta = &_transitionAnalysisPool[numUniqueTransitions++];

            if (!sta->analyzeSweepTransition(transitionStartIndex, prevLoopIndex, _executor)) {
                return sweepHangFailure(_executor);
            }

            SweepTransition newTransition(sta, prevLoopStartIndex);
            tg.addTransitionForExit(exitIndex, newTransition);
        }

        prevLoopIndex = loopIndex;
        prevLoopStartIndex = (rotationEquivalenceOffset != 0
                              ? loopBlock->getLoopPeriod() - rotationEquivalenceOffset
                              : 0);
        numSweeps++;
    }

    if (numSweeps < 4) {
        // The pattern is too short
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
        if (val == 0 || !group.loop().isExitValue(val)) {
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
    bool sweepMakesPersistentChange = (_sweepValueChangeType != SweepValueChangeType::NO_CHANGE);
    dp += delta;
    while (*dp) {
        if (_possibleSweepExitValues.find(*dp) != _possibleSweepExitValues.end()) {
            // Found end of sweep at other end
            break;
        }

        if (sweepMakesPersistentChange && canSweepChangeValueTowardsZero(*dp)) {
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

bool SweepHangDetector::analyzeHangBehaviour() {
    const RunSummary& runSummary = _executor.getRunSummary();

    if (runSummary.getNumRunBlocks() <= 8) {
        // The run should contain two full sweeps preceded by a loop: L1 (T1 L0 T0 L1) (T1 L0 T0 L1)
        // Note, transitions are named after the loop that precedes it (as they depend on the exit
        // of that loop).
        return false;
    }

    for (SweepTransitionGroup &tg : _transitionGroups) {
        tg.clear();
    }

//    _executor.getInterpretedProgram().dump();
//    _executor.dumpExecutionState();

    if (!analyzeLoops()) {
        return false;
    }

//    _executor.getInterpretedProgram().dump();
//    _executor.dumpExecutionState();

    if (!analyzeTransitions()) {
        return false;
    }

    //((const ExhaustiveSearcher &)_executor).getProgram().dumpWeb();

    if (!analyzeTransitionGroups()) {
        return failed(_executor);
    }

//    _executor.getInterpretedProgram().dump();
//    _executor.dumpExecutionState();
//    dump();

    return true;
}

Trilian SweepHangDetector::proofHang() {
    const Data& data = _executor.getData();
    DataPointer dp1 = data.getDataPointer();

//    data.dump();

    DataPointer dp0 = dp1; // Initial value
    if (_transitionGroups[1].endType() == SweepEndType::FIXED_GROWING) {
        dp0 = findAppendixStart(dp0, _transitionGroups[1]);
    }

    if (!scanSweepSequence(dp0, _transitionGroups[1].locatedAtRight())) {
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
    os << "Loop #0" << std::endl;
    os << detector._transitionGroups[0] << std::endl << std::endl;

    os << "Loop #1" << std::endl;
    os << detector._transitionGroups[1] << std::endl << std::endl;

    return os;
}
