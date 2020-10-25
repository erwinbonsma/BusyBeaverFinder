//
//  StaticSweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 17/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticSweepHangDetector.h"

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
    return false;
}


bool SweepLoopAnalysis::isExitValue(int value) {
    return std::find(_exitValues.begin(), _exitValues.end(), value) != _exitValues.end();
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

    _exitValues.clear();
    for (int i = loopSize(); --i >= 0; ) {
        if (exit(i).exitWindow == ExitWindow::ANYTIME) {
            _exitValues.push_back(exit(i).exitCondition.value());

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
    for (int value : sta._exitValues) {
        if (isFirst) {
            isFirst = false;
        } else {
            os << ", ";
        }
        os << value;
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

    _positionIsFixed = true;
    _sweepDeltaSign = _loop.deltaSign();
    // Check how the value that caused loop exit can be changed by any of the transition.
    for (auto kv : _transitions) {
        const SweepTransitionAnalysis *transition = kv.second;
        int finalValue = transition->dataDeltas().deltaAt(0);

        if (!_loop.isExitValue(finalValue)) {
            _positionIsFixed = false;

//            int sgn = sign(finalValue);
//            if (_sweepDeltaSign != 0 && sgn != _sweepDeltaSign) {
//                return false;
//            }
//            _sweepDeltaSign = sgn;
        }
    }

    _outsideDeltas.clear();
    for (auto kv : _transitions) {
        const SweepTransitionAnalysis *transition = kv.second;

        for (const DataDelta& dd : transition->dataDeltas()) {
            if (dd.dpOffset() != 0) {
                bool insideSweep = (dd.dpOffset() < 0) == _locatedAtRight;
                if (insideSweep) {
                    int sgn = sign(dd.delta());
                    if (_sweepDeltaSign != 0 && sgn != _sweepDeltaSign) {
                        return failed(*this);
                    }
                    _sweepDeltaSign = sgn;
                } else {
                    if (_positionIsFixed) {
                        // Check all deltas at specific offset have same sign
                        int delta = _outsideDeltas.deltaAt(dd.dpOffset());
                        if (delta == 0) {
                            _outsideDeltas.addDelta(dd.dpOffset(), sign(dd.delta()));
                        } else if (delta * dd.delta() < 0) {
                            return failed(*this);
                        }
                    } else {
                        // TODO: Support bigger extension of sweep and/or appendix
                        // Should check that there are no discontinuities (possible exits)
                        return failed(*this);
                    }
                }
            }
        }
    }

    return true;
}

std::ostream &operator<<(std::ostream &os, const SweepTransitionGroup &group) {
    os << group._loop;

    auto iter = group._transitions.begin();
    while (iter != group._transitions.end()) {
        os << "  Exit at " << iter->first << ": " << *(iter->second) << std::endl;
        ++iter;
    }

    os << "at right = " << group.locatedAtRight()
    << ", is fixed = " << group.positionIsFixed();

    return os;
}

StaticSweepHangDetector::StaticSweepHangDetector(const ProgramExecutor& executor)
    : StaticHangDetector(executor) {}

bool StaticSweepHangDetector::analyzeLoops() {
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

bool StaticSweepHangDetector::analyzeTransitions() {
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
        int exitInstruction = loopLen % loopBlock->getLoopPeriod();

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

bool StaticSweepHangDetector::analyzeTransitionGroups() {
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

bool StaticSweepHangDetector::scanSweepSequence(DataPointer &dp, SweepLoopAnalysis &sweepLoop) {
    const Data& data = _executor.getData();

    // For now, scan all values as the values that are skipped now may be expected during a next
    // sweep.
    // TODO: Make analysis smarter.
    int delta = sign(sweepLoop.dataPointerDelta());

    DataPointer dpEnd = (delta > 0) ? data.getMaxDataP() : data.getMinDataP();
    SweepLoopAnalysis &loop0 = _transitionGroups[0].loop(), &loop1 = _transitionGroups[1].loop();

    // DP is at the other side of the sweep. Find the other end of the sweep.
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

bool StaticSweepHangDetector::onlyZeroesAhead(DataPointer &dp, bool atRight) {
    const Data& data = _executor.getData();
    int delta = atRight ? 1 : -1;
    DataPointer dpEnd = atRight ? data.getMaxDataP() : data.getMinDataP();

    while (true) {
        if (*dp) {
            return failed(_executor);
        }

        if (dp == dpEnd) {
            break;
        }

        dp += delta;
    }

    return true;
}

bool StaticSweepHangDetector::shouldCheckNow(bool loopContinues) {
    // Should wait for the sweep-loop to finish
    return !loopContinues;
}

bool StaticSweepHangDetector::analyzeHangBehaviour() {
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

    if (!analyzeTransitions()) {
        return false;
    }

    if (!analyzeTransitionGroups()) {
        return failed(_executor);
    }

    return true;
}

Trilian StaticSweepHangDetector::proofHang() {
    const Data& data = _executor.getData();
    DataPointer dp1 = data.getDataPointer();
    DataPointer dp0 = dp1; // Initial value

    if (!scanSweepSequence(dp0, _transitionGroups[0].loop())) {
        return Trilian::MAYBE;
    }

    for (int i = 0; i < 2; ++i) {
        SweepTransitionGroup &tg = _transitionGroups[i];
        DataPointer dp = (i == 0) ? dp0 : dp1;

        if (tg.positionIsFixed()) {
            // Check all outside deltas move values away from zero
            for (const DataDelta &dd : tg.outsideDeltas()) {
                if (dd.delta() * data.valueAt(dp, dd.dpOffset()) < 0) {
                    return Trilian::MAYBE;
                }
            }
        } else {
            // Check all outside values are zero
            if (!onlyZeroesAhead(dp, tg.locatedAtRight())) {
                return Trilian::MAYBE;
            }
        }
    }

//    _executor.getRunSummary().dump();
//    _executor.getMetaRunSummary().dump();
//    dump();
//    _executor.getData().dump();
//    _executor.getInterpretedProgram().dump();

    return Trilian::YES;
}

void StaticSweepHangDetector::dump() const {
    std::cout << *this << std::endl;
}

std::ostream &operator<<(std::ostream &os, const StaticSweepHangDetector &detector) {
    os << "Loop #0" << std::endl;
    os << detector._transitionGroups[0] << std::endl;

    os << "Loop #1" << std::endl;
    os << detector._transitionGroups[1] << std::endl;

    return os;
}
