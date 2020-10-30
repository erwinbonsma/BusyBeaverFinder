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

int numFailed = 0;
bool failed(const ProgramExecutor& executor) {
    numFailed++;
    // executor.dumpExecutionState();
    return false;
}

bool failed(SweepTransitionGroup& tg) {
//    std::cout << tg << std::endl;
    numFailed++;
    return false;
}


bool SweepLoopAnalysis::isExitValue(int value) const {
    return _exitMap.find(value) != _exitMap.end();
}

int SweepLoopAnalysis::numberOfExitsForValue(int value) const {
    return (int)_exitMap.count(value);
}

bool SweepLoopAnalysis::analyzeSweepLoop(const RunBlock* runBlock,
                                         const ProgramExecutor& executor) {
    if (!analyzeLoop(executor.getInterpretedProgram(),
                     executor.getRunSummary(),
                     runBlock->getStartIndex(),
                     runBlock->getLoopPeriod())) {
        return failed(executor);
    }

//    if (abs(dataPointerDelta()) != 1) {
//        // TODO: Support loops that move more than one cell per iteration
//        return failed(executor);
//    }

//    if (numBootstrapCycles() > 0) {
//        // TODO: Support loops with bootstrap
//        return failed(executor);
//    }

    _sweepValueChangeType = SweepValueChangeType::NO_CHANGE; // Initial value
    _sweepValueChange = 0;
    for (int i = numDataDeltas(); --i >= 0; ) {
        int delta = dataDeltaAt(i).delta();
        switch (_sweepValueChangeType) {
            case SweepValueChangeType::NO_CHANGE:
                _sweepValueChangeType = SweepValueChangeType::UNIFORM_CHANGE;
                _sweepValueChange = delta;
                break;
            case SweepValueChangeType::UNIFORM_CHANGE:
            case SweepValueChangeType::MULTIPLE_ALIGNED_CHANGES:
                if (_sweepValueChange != delta) {
                    _sweepValueChangeType =
                        sign(delta) == sign(_sweepValueChange)
                        ? SweepValueChangeType::MULTIPLE_ALIGNED_CHANGES
                        : SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES;
                }
                break;
            case SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES:
                // void
                break;
        }
    }

    _exitMap.clear();
    _requiresFixedInput = false;
    for (int i = loopSize(); --i >= 0; ) {
        if (exit(i).exitWindow == ExitWindow::ANYTIME) {
            if (exitsOnZero(i)) {
                _exitMap.insert({exit(i).exitCondition.value(), i});
            } else {
                // The loop exits on a non-zero value. This means that the loop only loops when it
                // consumes a specific value.
                _requiresFixedInput = true;
            }
        }
    }

    return true;
}

std::ostream &operator<<(std::ostream &os, const SweepLoopAnalysis& sta) {
    os << (const LoopAnalysis&)sta;
    os << "Exit values: ";
    bool isFirst = true;
    for (auto pair : sta._exitMap) {
        if (isFirst) {
            isFirst = false;
        } else {
            os << ", ";
        }
        os << pair.first << "@" << pair.second;
    }
    os << std::endl;
    if (sta._requiresFixedInput) {
        os << "Requires fixed input!" << std::endl;
    }

    return os;
}

bool SweepTransitionAnalysis::analyzeSweepTransition(int startIndex, int endIndex,
                                                     const ProgramExecutor& executor) {
    const RunSummary& runSummary = executor.getRunSummary();
    const InterpretedProgram& interpretedProgram = executor.getInterpretedProgram();

    // The instructions comprising the transition sequence
    int pbStart = runSummary.runBlockAt(startIndex)->getStartIndex();
    int numProgramBlocks = runSummary.runBlockAt(endIndex)->getStartIndex() - pbStart;

    if (!analyzeSequence(interpretedProgram, runSummary, pbStart, numProgramBlocks)) {
        return failed(executor);
    }

    return true;
}

bool SweepTransitionAnalysis::transitionEquals(int startIndex, int endIndex,
                                               const ProgramExecutor& executor) const {
    const RunSummary& runSummary = executor.getRunSummary();
    const InterpretedProgram& program = executor.getInterpretedProgram();

    // The instructions comprising the transition sequence
    int pbStart = runSummary.runBlockAt(startIndex)->getStartIndex();
    int numProgramBlocks = runSummary.runBlockAt(endIndex)->getStartIndex() - pbStart;

    if (numProgramBlocks != sequenceSize()) {
        return false;
    }

    for (int i = numProgramBlocks; --i >= 0; ) {
        if (program.indexOf(programBlockAt(i)) != runSummary.programBlockIndexAt(pbStart + i)) {
            return false;
        }
    }

    return true;
}

void SweepTransitionAnalysis::dump() const {
    std::cout << *this << std::endl;
}

std::ostream &operator<<(std::ostream &os, const SweepTransitionAnalysis& sta) {
    os << (const SequenceAnalysis&)sta;

    return os;
}

bool SweepTransitionGroup::analyzeLoop(int runBlockIndex, const ProgramExecutor& executor) {
    _loopRunBlock = executor.getRunSummary().runBlockAt(runBlockIndex);

    if (!_loop.analyzeSweepLoop(_loopRunBlock, executor)) {
        return false;
    }

    if (_loop.sweepValueChangeType() == SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES) {
        // Not (yet?) supported
        return failed(*this);
    }

    _locatedAtRight = _loop.dataPointerDelta() > 0;
    _transitions.clear();

    return true;
}

int SweepTransitionGroup::numberOfTransitionsForExitValue(int value) {
    int count = 0;

    for (auto kv : _transitions) {
        int exitInstruction = kv.first;
        if (_loop.exit(exitInstruction).exitCondition.isTrueForValue(value)) {
            ++count;
        }
    }

    return count;
}

bool SweepTransitionGroup::determineSweepEndType() {
    bool exitToExit = false;
    bool exitToNonExit = false;
    bool nonExitToExit = false;

    for (auto kv : _transitions) {
        const SweepTransitionAnalysis *transition = kv.second;
        int finalValue = transition->dataDeltas().deltaAt(0);
        int exitCount = _loop.numberOfExitsForValue(finalValue);

        if (exitCount > 0) {
            // The value that caused the loop to exit and continue at the given transition, is
            // converted (by the transition) to another value that causes a loop exit (in a later
            // sweep).
            exitToExit = true;

            int numTransitions = numberOfTransitionsForExitValue(finalValue);
            if (numTransitions != exitCount) {
                // Not all possible exits are (yet) followed by a transition that continues the
                // sweep. We can therefore not yet conclude this is a hang.
                return failed(*this);
            }
        } else {
            // This transition extends the sequence. It results in a value that does not cause the
            // loop to exit in a future sweep.
            exitToNonExit = true;

            // Also verify that it cannot cause the loop sweeping in the other direction to exit.
            // Note: although it was followed by a successful execution of the loop at least once
            // already, this does not proof it cannot cause that loop to exit. It could be that the
            // value was not yet seen by the instruction whose exit it can cause.
            if (_sibling->loop().isExitValue(finalValue)) {
                return failed(*this);
            }

            int sgn = sign(finalValue);
            if (
                sgn != 0 && (
                    sgn == -sign( _loop.sweepValueChange() ) ||
                    sgn == -sign( _sibling->_loop.sweepValueChange() )
                )
            ) {
                // Although it cannot directly cause the loop to exit, it is modified by the sweeps
                // towards zero, which will likely cause it to exit the loop again.
                // TODO: Refine this check
                nonExitToExit = true;
            }
        }
    }

    // When abs(delta DP) > 1, not only change at DP is relevant. See e.g. FakeSweep.
    // TODO: Extend recognition accordingly.

    if (!exitToExit && exitToNonExit && !nonExitToExit) {
        _sweepEndType = SweepEndType::STEADY_GROWTH;
    } else if (exitToExit && !exitToNonExit) {
        _sweepEndType = SweepEndType::FIXED_POINT;
    } else if (exitToExit && exitToNonExit && !nonExitToExit) {
        _sweepEndType = SweepEndType::IRREGULAR_GROWTH;
    } else if (exitToNonExit && nonExitToExit) {
        _sweepEndType = SweepEndType::FIXED_GROWING;
    } else {
        // Unsupported sweep end type
        return false;
    }

    bool zeroValueShouldContinueSweep = (
        _sweepEndType == SweepEndType::STEADY_GROWTH ||
        _sweepEndType == SweepEndType::IRREGULAR_GROWTH ||
        _sweepEndType == SweepEndType::FIXED_GROWING
    );
    if (zeroValueShouldContinueSweep) {
        // A zero value should always ensure a reversal of the sweep. Check if this is so.
        int numZeroExits = _loop.numberOfExitsForValue(0);
        if (numZeroExits == 0 || numberOfTransitionsForExitValue(0) != numZeroExits) {
            return failed(*this);
        }
    }

    return true;
}

bool SweepTransitionGroup::analyzeGroup() {
//    // Check there is a transition for each anytime-exit
//    for (int i = _loop.loopSize(); --i >= 0; ) {
//        if (_loop.exit(i).exitWindow == ExitWindow::ANYTIME) {
//            if (_transitions.find(i) == _transitions.end()) {
//                // TODO: Proof that this exit can never be triggered
//                // This requires proving that the exit value can never be added to the sweep body
//                // nor to the transition appendix.
//                return failed(*this);
//            }
//        }
//    }

    if (!determineSweepEndType()) {
        return false;
    }

//    std::cout << *this << std::endl;

    int sweepDeltaSign = sign(_loop.sweepValueChange());
    _outsideDeltas.clear();
    for (auto kv : _transitions) {
        const SweepTransitionAnalysis *transition = kv.second;

        for (const DataDelta& dd : transition->dataDeltas()) {
            if (dd.dpOffset() != 0) {
                bool insideSweep = (dd.dpOffset() < 0) == _locatedAtRight;
                int sgn = sign(dd.delta());
                if (insideSweep) {
                    if (sweepDeltaSign != 0 && sgn != sweepDeltaSign) {
                        // TODO: Refine or remove this check
                        return failed(*this);
                    }
                    sweepDeltaSign = sgn;
                } else {
                    switch (_sweepEndType) {
                        case SweepEndType::FIXED_POINT: {
                            // Check all deltas at specific offset have same sign
                            int delta = _outsideDeltas.deltaAt(dd.dpOffset());
                            if (delta == 0) {
                                _outsideDeltas.addDelta(dd.dpOffset(), sgn);
                            } else if (delta != sgn) {
                                return failed(*this);
                            }

                            break;
                        }
                        case SweepEndType::FIXED_GROWING:
                            if (!_loop.isExitValue(dd.delta())) {
                                return failed(*this);
                            }
                            break;
                        case SweepEndType::STEADY_GROWTH:
                        case SweepEndType::IRREGULAR_GROWTH:
                            // For now, assume only transition point extends the sweep sequence
                            return failed(*this);
                    }
                }
            }
        }
    }

    return true;
}

Trilian SweepTransitionGroup::proofHang(DataPointer dp, const Data& data) {
    switch (_sweepEndType) {
        case SweepEndType::FIXED_POINT:
            // Check all outside deltas move values away from zero
            for (const DataDelta &dd : _outsideDeltas) {
                if (dd.delta() * data.valueAt(dp, dd.dpOffset()) < 0) {
                    return Trilian::MAYBE;
                }
            }
            break;
        case SweepEndType::FIXED_GROWING: {
            int delta = locatedAtRight() ? 1 : -1;
            // Skip all appendix values
            while (true) {
                int val = data.valueAt(dp, delta);
                if (val == 0 || !_loop.isExitValue(val)) {
                    break;
                }
                dp += delta;
            }
        }
            // Fall through
        case SweepEndType::STEADY_GROWTH:
        case SweepEndType::IRREGULAR_GROWTH:
            if (!data.onlyZerosAhead(dp, locatedAtRight())) {
                return Trilian::MAYBE;
            }
            break;
    }
    return Trilian::YES;
}

std::ostream &operator<<(std::ostream &os, const SweepTransitionGroup &group) {
    os << group._loop;

    auto iter = group._transitions.begin();
    while (iter != group._transitions.end()) {
        os << "  Exit at " << iter->first << ": " << *(iter->second) << std::endl;
        ++iter;
    }

    os << "Type = ";
    switch (group.endType()) {
        case SweepEndType::STEADY_GROWTH: os << "Steady growth"; break;
        case SweepEndType::IRREGULAR_GROWTH: os << "Irregular growth"; break;
        case SweepEndType::FIXED_POINT: os << "Fixed point"; break;
        case SweepEndType::FIXED_GROWING: os << "Fixed growing"; break;
    }

    return os;
}

SweepHangDetector::SweepHangDetector(const ProgramExecutor& executor)
: HangDetector(executor) {
    _transitionGroups[0].initSibling(&_transitionGroups[1]);
    _transitionGroups[1].initSibling(&_transitionGroups[0]);}

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

bool SweepHangDetector::analyzeLoops() {
    const RunSummary& runSummary = _executor.getRunSummary();
    SweepTransitionGroup *group = _transitionGroups;

    // Assume that the loop which just finished is one of the sweep loops
    if (runSummary.getLoopIteration() < MIN_ITERATIONS_LAST_SWEEP_LOOP) {
        return failed(_executor);
    }
    int runBlockIndexLoop1 = runSummary.getNumRunBlocks() - 1;
    if (!group[1].analyzeLoop(runBlockIndexLoop1, _executor)) {
        return false;
    }

    int runBlockIndexTransition0 = findPrecedingTransitionStart(runBlockIndexLoop1);
    if (runBlockIndexTransition0 == 0) {
        // There is no sweep loop preceding the transition
        return failed(_executor);
    }
    int runBlockIndexLoop0 = runBlockIndexTransition0 - 1;
    if (!group[0].analyzeLoop(runBlockIndexLoop0, _executor)) {
        return false;
    }

    // Both loops should move in opposite directions
    if (group[0].locatedAtRight() == group[1].locatedAtRight()) {
        return failed(_executor);
    }

    auto loop0 = group[0].loop(), loop1 = group[1].loop();

    int sgn0 = sign(loop0.sweepValueChange()), sgn1 = sign(loop1.sweepValueChange());
    if (sgn1 == 0 || sgn0 == sgn1) {
        _sweepDeltaSign = sgn0;
    } else if (sgn0 == 0) {
        _sweepDeltaSign = sgn1;
    } else if (
        loop0.sweepValueChangeType() == SweepValueChangeType::UNIFORM_CHANGE &&
        loop1.sweepValueChangeType() == SweepValueChangeType::UNIFORM_CHANGE &&
        loop0.sweepValueChange() == -loop1.sweepValueChange()
    ) {
        _sweepDeltaSign = 0;
    } else {
        // Both loops make opposite changes that do not fully cancel out. This is not supported yet
        return failed(_executor);
    }


    if (loop0.requiresFixedInput() || loop1.requiresFixedInput()) {
        // The changes of both loops, if any, should cancel each other out.
        if (_sweepDeltaSign != 0) {
            return failed(_executor);
        }

    }

    return true;
}

bool SweepHangDetector::analyzeTransitions() {
    const RunSummary& runSummary = _executor.getRunSummary();
    const RunSummary& metaRunSummary = _executor.getMetaRunSummary();
    int metaLoopStartIndex = metaRunSummary.getLastRunBlock()->getStartIndex();
    int prevLoopIndex = runSummary.getNumRunBlocks() - 1;
    int numSweeps = 0, numUniqueTransitions = 0;

    while (prevLoopIndex > metaLoopStartIndex) {
        int transitionStartIndex = findPrecedingTransitionStart(prevLoopIndex);
        if (transitionStartIndex < metaLoopStartIndex) {
            // No more run blocks remain
            break;
        }

        int loopIndex = transitionStartIndex - 1;
        const RunBlock* loopBlock = runSummary.runBlockAt(loopIndex);
        assert(loopBlock->isLoop());

        int j = numSweeps % 2;
        SweepTransitionGroup &tg = _transitionGroups[j];

        if (loopBlock->getSequenceIndex() != tg.loopRunBlock()->getSequenceIndex()) {
            // Sequence does not follow expected sweep pattern anymore
            break;
        }

        int loopLen = runSummary.getRunBlockLength(loopIndex);
        int exitInstruction = (loopLen - 1) % loopBlock->getLoopPeriod();
        const SweepTransitionAnalysis* sta = tg.transitionForExit(exitInstruction);
        if (sta != nullptr) {
            // Check that the transition is identical
            if (!sta->transitionEquals(transitionStartIndex, prevLoopIndex, _executor)) {
                // Transition does not match. This may be due to start-up effects. This could still
                // be a hang.
                break;
            }
        } else {
            // This is the first transition that follows the given loop exit
            assert(numUniqueTransitions < MAX_UNIQUE_TRANSITIONS_PER_SWEEP);
            SweepTransitionAnalysis *sa = &_transitionPool[numUniqueTransitions++];

            if (!sa->analyzeSweepTransition(transitionStartIndex, prevLoopIndex, _executor)) {
                return failed(_executor);
            }

            tg.addTransitionForExit(sa, exitInstruction);
        }

        prevLoopIndex = loopIndex;
        numSweeps++;
    }

    if (numSweeps < 4) {
        // The pattern is too short
        return failed(_executor);
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
    const SweepLoopAnalysis &loop0 = _transitionGroups[0].loop();
    const SweepLoopAnalysis &loop1 = _transitionGroups[1].loop();

    // DP is at one side of the sweep. Find the other end of the sweep.
    dp += delta;
    while (*dp) {
        if (loop0.isExitValue(*dp) || loop1.isExitValue(*dp)) {
            // Found end of sweep at other end
            break;
        }

        if (_sweepDeltaSign * sign(*dp) < 0) {
            // The sweep makes changes to the sequence that move some values towards zero
            return failed(_executor);
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
