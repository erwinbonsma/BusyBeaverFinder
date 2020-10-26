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

    _deltaSign = 0;
    for (int i = numDataDeltas(); --i >= 0; ) {
        int sgn = sign(dataDeltaAt(i).delta());

        if (_deltaSign != 0 && _deltaSign != sgn) {
            // TODO?: Support sweep loops that make changes in opposite directions
            return failed(executor);
        }
        _deltaSign = sgn;
    }

    _exitMap.clear();
    for (int i = loopSize(); --i >= 0; ) {
        if (exit(i).exitWindow == ExitWindow::ANYTIME) {
            _exitMap.insert({exit(i).exitCondition.value(), i});

            if (!exitsOnZero(i)) {
                // TODO?: Support loops that exit on non-zero
                return failed(executor);
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

    return os;
}

bool SweepTransitionAnalysis::analyzeSweepTransition(const RunBlock* runBlock, bool atRight,
                                                     const ProgramExecutor& executor) {
    const RunSummary& runSummary = executor.getRunSummary();
    const InterpretedProgram& interpretedProgram = executor.getInterpretedProgram();

    // The instructions comprising the (last) transition sequence
    int startIndex = runBlock->getStartIndex();
    int len = (runBlock + 1)->getStartIndex() - startIndex;

    if (!analyzeSequence(interpretedProgram, runSummary, startIndex, len)) {
        return failed(executor);
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

bool SweepTransitionGroup::analyzeLoop(const RunBlock* runBlock, const ProgramExecutor& executor) {
    _loopRunBlock = runBlock;

    if (!runBlock->isLoop()) {
        return false;
    }

    if (!_loop.analyzeSweepLoop(_loopRunBlock, executor)) {
        return false;
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
            if (sgn != 0 && (sgn == -_loop.deltaSign() || sgn == -_sibling->_loop.deltaSign())) {
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

    _sweepDeltaSign = _loop.deltaSign();
    _outsideDeltas.clear();
    for (auto kv : _transitions) {
        const SweepTransitionAnalysis *transition = kv.second;

        for (const DataDelta& dd : transition->dataDeltas()) {
            if (dd.dpOffset() != 0) {
                bool insideSweep = (dd.dpOffset() < 0) == _locatedAtRight;
                int sgn = sign(dd.delta());
                if (insideSweep) {
                    if (_sweepDeltaSign != 0 && sgn != _sweepDeltaSign) {
                        return failed(*this);
                    }
                    _sweepDeltaSign = sgn;
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

bool SweepHangDetector::analyzeLoops() {
    // Assume that the loop which just finished is one of the sweep loops
    const RunSummary& runSummary = _executor.getRunSummary();
    SweepTransitionGroup *group = _transitionGroups;

    const RunBlock *loop1RunBlock = runSummary.getLastRunBlock();
    if (!group[1].analyzeLoop(loop1RunBlock, _executor)) {
        return false;
    }
    if (!group[0].analyzeLoop(loop1RunBlock - 2, _executor)) {
        return false;
    }

    // Both loops should move in opposite directions
    if (group[0].locatedAtRight() == group[1].locatedAtRight()) {
        return failed(_executor);
    }

    return true;
}

bool SweepHangDetector::analyzeTransitions() {
    const RunSummary& runSummary = _executor.getRunSummary();
    int i = runSummary.getNumRunBlocks() - 2;
    int numTransitions = 0, numUniqueTransitions = 0;

    while (i > 0) {
        const RunBlock* transitionBlock = runSummary.runBlockAt(i);
        const RunBlock* loopBlock = runSummary.runBlockAt(i - 1);
        int j = numTransitions % 2;
        SweepTransitionGroup &tg = _transitionGroups[j];

        if (
            !loopBlock->isLoop() ||
            loopBlock->getSequenceIndex() != tg.loopRunBlock()->getSequenceIndex()
        ) {
            // Loops do not follow expected sweep pattern
            break;
        }

        int loopLen = runSummary.getRunBlockLength(i - 1);
        int exitInstruction = (loopLen - 1) % loopBlock->getLoopPeriod();

        if (!tg.hasTransitionForExit(exitInstruction)) {
            // This is the first transition that follows the given loop exit
            assert(numUniqueTransitions < MAX_UNIQUE_TRANSITIONS_PER_SWEEP);
            SweepTransitionAnalysis *sa = &_transitionPool[numUniqueTransitions++];

            if (!sa->analyzeSweepTransition(transitionBlock, tg.locatedAtRight(), _executor)) {
                return false;
            }

            tg.addTransitionForExit(sa, exitInstruction);
        }

        i -= 2;
        numTransitions++;
    }

    if (numTransitions < 4) {
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

    SweepTransitionGroup *groups = _transitionGroups;
    if (groups[0].sweepDeltaSign() * groups[1].sweepDeltaSign() == -1) {
        // TODO?: Support loops that makes changes to the sequence in opposite directions
        return failed(_executor);
    }
    _sweepDeltaSign = groups[(int)(groups[0].sweepDeltaSign() == 0)].sweepDeltaSign();

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
    return !loopContinues;
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
