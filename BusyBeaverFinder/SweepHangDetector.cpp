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

std::ostream &operator<<(std::ostream &os, SweepEndType endType) {
    switch (endType) {
        case SweepEndType::STEADY_GROWTH: os << "Steady growth"; break;
        case SweepEndType::IRREGULAR_GROWTH: os << "Irregular growth"; break;
        case SweepEndType::FIXED_POINT_CONSTANT_VALUE: os << "Fixed point, constant value"; break;
        case SweepEndType::FIXED_POINT_MULTIPLE_VALUES: os << "Fixed point, multiple values"; break;
        case SweepEndType::FIXED_POINT_INCREASING_VALUE: os << "Fixed point, increasing value"; break;
        case SweepEndType::FIXED_POINT_DECREASING_VALUE: os << "Fixed point, decreasing value"; break;
        case SweepEndType::FIXED_GROWING: os << "Fixed growing"; break;
        case SweepEndType::UNKNOWN: break;
    }

    return os;
}

namespace std {
size_t hash<SweepLoopExit>::operator()(const SweepLoopExit& sle) const {
    return (sle.loop->loopRunBlock()->getSequenceIndex() << 8) ^ sle.exitInstructionIndex;
}
}

bool SweepLoopAnalysis::isExitValue(int value) const {
    return _exitMap.find(value) != _exitMap.end();
}

int SweepLoopAnalysis::numberOfExitsForValue(int value) const {
    return (int)_exitMap.count(value);
}

DataDeltas tmpDataDeltasIndirectExits;
bool SweepLoopAnalysis::hasIndirectExitsForValueAfterExit(int value, int exitInstruction) const {
    DataDeltas &dataDeltas = tmpDataDeltasIndirectExits;
    int dpDelta = 0;

    dataDeltas.clear();
    // Fully bootstrap the loop
    int maxIteration = numBootstrapCycles();
    for (int iteration = 0; iteration <= maxIteration; iteration++) {
        // Execute the last iteration until (inclusive) the exit instruction
        for (int instruction = 0; instruction < loopSize(); instruction++) {
            const ProgramBlock *pb = programBlockAt(instruction);

            if (pb->isDelta()) {
                dataDeltas.updateDelta(dpDelta, pb->getInstructionAmount());
            } else {
                dpDelta += pb->getInstructionAmount();
            }

//            if (instruction == exitInstruction) {
//                std::cout << "Iteration " << iteration
//                    << " dpOffset = " << dpDelta
//                    << ": " << dataDeltas << std::endl;
//            }

            if (instruction == exitInstruction && iteration == maxIteration) {
                int dpDeltaStartIteration = dpDelta - effectiveResultAt(exitInstruction).dpOffset();
                int dpMin = dpDeltaStartIteration + minDp();
                int dpMax = dpDeltaStartIteration + maxDp();
                for (DataDelta dd : dataDeltas) {
                    bool insideSweep = sign(dpDelta - dd.dpOffset()) == sign(dataPointerDelta());
                    bool isBootstrapResidu = dd.dpOffset() < dpMin || dd.dpOffset() > dpMax;
                    int finalValue = value + dd.delta() - _sweepValueChange;
                    if (insideSweep && !isBootstrapResidu && isExitValue(finalValue)) {
                        return true;
                    }
                }

                return false;
            }
        }
    }

    assert(false);
    return false;
}

bool SweepLoopAnalysis::hasIndirectExitsForValue(int value) const {
    // Assumption to simplify analysis.
    assert(_sweepValueChangeType == SweepValueChangeType::NO_CHANGE ||
           _sweepValueChangeType == SweepValueChangeType::UNIFORM_CHANGE);

    for (int instructionIndex = loopSize(); --instructionIndex >= 0; ) {
        const LoopExit& loopExit = exit(instructionIndex);

        if (loopExit.exitWindow == ExitWindow::ANYTIME) {
            // Determine what all inside-sweep deltas can be when this exit is taken
            if (hasIndirectExitsForValueAfterExit(value, instructionIndex)) {
                return true;
            }
        }
    }

    return false;
}

bool SweepLoopAnalysis::analyzeSweepLoop(const RunBlock* runBlock,
                                         const ProgramExecutor& executor) {
    _loopRunBlock = runBlock;

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
    int numUniformChanges = 0;
    for (int i = numDataDeltas(); --i >= 0; ) {
        int delta = dataDeltaAt(i).delta();
        switch (_sweepValueChangeType) {
            case SweepValueChangeType::NO_CHANGE:
                _sweepValueChangeType = SweepValueChangeType::UNIFORM_CHANGE;
                _sweepValueChange = delta;
                numUniformChanges = 1;
                break;
            case SweepValueChangeType::UNIFORM_CHANGE:
                if (_sweepValueChange == delta) {
                    ++numUniformChanges;
                }
                // Fall through
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
    if (
        _sweepValueChangeType == SweepValueChangeType::UNIFORM_CHANGE &&
        numUniformChanges != abs(dataPointerDelta())
    ) {
        // Not all values change, so the change is not actually uniform.
        _sweepValueChangeType = SweepValueChangeType::MULTIPLE_ALIGNED_CHANGES;
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

bool SweepLoopExit::exitsOnZero() const {
    return loopExit().exitCondition.getOperator() == Operator::EQUALS;
}

void SweepTransitionGroup::init(const SweepHangDetector *parent,
                                const SweepTransitionGroup *sibling) {
    _parent = parent;
    _sibling = sibling;
}

void SweepTransitionGroup::clear() {
    _loops.clear();
    _transitions.clear();
    _sweepEndType = SweepEndType::UNKNOWN;
}

bool SweepTransitionGroup::analyzeLoop(SweepLoopAnalysis* loopAnalysis, int runBlockIndex,
                                       const ProgramExecutor& executor) {
    const RunBlock *loopRunBlock = executor.getRunSummary().runBlockAt(runBlockIndex);

    if (!loopAnalysis->analyzeSweepLoop(loopRunBlock, executor)) {
        return false;
    }

    if (loopAnalysis->sweepValueChangeType() == SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES) {
        // Not (yet?) supported
        return failed(*this);
    }

    _locatedAtRight = loopAnalysis->dataPointerDelta() > 0;
    _loops[loopRunBlock->getSequenceIndex()] = loopAnalysis;

    return true;
}

int SweepTransitionGroup::numberOfTransitionsForExitValue(int value) const {
    int count = 0;

    for (auto kv : _transitions) {
        SweepLoopExit sweepLoopExit = kv.first;
        const LoopExit &loopExit = sweepLoopExit.loop->exit(sweepLoopExit.exitInstructionIndex);

        if (loopExit.exitCondition.isTrueForValue(value)) {
            ++count;
        }
    }

    return count;
}

int SweepTransitionGroup::numberOfExitsForValue(int value) const {
    int count = 0;

    for (auto kv : _loops) {
        const SweepLoopAnalysis *loop = kv.second;
        count += loop->numberOfExitsForValue(value);
    }

    return count;
}

bool SweepTransitionGroup::hasIndirectExitsForValue(int value) const {
    assert(_parent->combinedSweepValueChange() == 0);

    for (auto kv : _loops) {
        const SweepLoopAnalysis *loop = kv.second;
        if (loop->hasIndirectExitsForValue(value)) {
            return true;
        }
    }

    return false;
}

bool SweepTransitionGroup::determineSweepEndType() {
    bool exitToExit = false;
    bool exitToNonExit = false;
    bool nonExitToExit = false;
    int numPositiveDeltas = 0, numNegativeDeltas = 0;
    int numNonZeroExits = 0;

//    std::cout << *this;

    for (auto kv : _transitions) {
        const SweepLoopExit &sweepLoopExit = kv.first;
        const SweepTransition &transition = kv.second;
        const SweepTransitionAnalysis *sta = transition.transition;

        // Determine delta after exit. For zero-exits, this is the final value
        int delta = sta->dataDeltas().deltaAt(0);
        delta += transition.nextLoop->deltaAt( -sta->dataPointerDelta() );

        // Determine how much the value changes compared to its initial value.
        int totalDelta = delta - sweepLoopExit.loopExit().exitCondition.value();

        if (totalDelta > 0) {
            ++numPositiveDeltas;
        } else if (totalDelta < 0) {
            ++numNegativeDeltas;
        }

        if (sweepLoopExit.exitsOnZero()) {
            int exitCount = numberOfExitsForValue(delta);

            if (exitCount > 0) {
                // The value that caused the loop to exit and continue at the given transition, is
                // converted (by the transition) to another value that causes a loop exit (in a
                // later sweep).
                exitToExit = true;

                int numTransitions = numberOfTransitionsForExitValue(delta);
                if (numTransitions != exitCount) {
                    // Not all possible exits are (yet) followed by a transition that continues the
                    // sweep. We can therefore not yet conclude this is a hang.
                    return failed(*this);
                }
            } else {
                // This transition extends the sequence. It results in a value that does not
                // (directly) cause the loop to exit in a future sweep.

                exitToNonExit = true;

                int sgn = sign(delta);
                int sgnCombinedSweepChange = sign(_parent->combinedSweepValueChange());
                if (sgn != 0 && sgn == -sgnCombinedSweepChange) {
                    // Although it cannot directly cause the loop to exit, it is modified by the
                    // sweeps towards zero, which will likely cause it to exit the loop again.
                    nonExitToExit = true;
                } else if (sgnCombinedSweepChange == 0 && hasIndirectExitsForValue(delta)) {
                    // Although the combined sweep does not result in value changes, loop exits
                    // still can change this value into one that causes an exit in a later sweep.
                    nonExitToExit = true;
                }
            }
        } else {
            // Loop exits on a non-zero value. The unusual case
            ++numNonZeroExits;
        }
    }

    // When abs(delta DP) > 1, not only change at DP is relevant. See e.g. FakeSweep.
    // TODO: Extend recognition accordingly.

    if (numNonZeroExits > 0) {
        if (numNonZeroExits < _transitions.size()) {
            // To facilitate analysis, when one exit is on a non-zero value, require that all are
            // TODO: Extend analysis when needed
            return failed(*this);
        }
        if (numNegativeDeltas > 0 && numPositiveDeltas > 0) {
            // To facilitate analysis, require that deltas are not in opposite directions.
            // TODO: Extend analysis when needed
            return failed(*this);
        }
        if (numPositiveDeltas > 0) {
            _sweepEndType = SweepEndType::FIXED_POINT_INCREASING_VALUE;
        } else if (numNegativeDeltas > 0) {
            _sweepEndType = SweepEndType::FIXED_POINT_DECREASING_VALUE;
        } else {
            _sweepEndType = SweepEndType::FIXED_POINT_CONSTANT_VALUE;
        }
    } else {
        if (!exitToExit && exitToNonExit && !nonExitToExit) {
            _sweepEndType = SweepEndType::STEADY_GROWTH;
        } else if (exitToExit && !exitToNonExit) {
            if (numPositiveDeltas > 0 || numNegativeDeltas > 0) {
                _sweepEndType = SweepEndType::FIXED_POINT_MULTIPLE_VALUES;
            } else {
                _sweepEndType = SweepEndType::FIXED_POINT_CONSTANT_VALUE;
            }
        } else if (exitToExit && exitToNonExit && !nonExitToExit) {
            _sweepEndType = SweepEndType::IRREGULAR_GROWTH;
        } else if (exitToNonExit && nonExitToExit) {
            _sweepEndType = SweepEndType::FIXED_GROWING;
            // Unsupported for regular sweeps
            return false;
        } else {
            // Unsupported sweep end type
            return false;
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

    _insideSweepTransitionDeltaSign = 0;
    _outsideDeltas.clear();
    for (auto kv : _transitions) {
        const SweepTransition transition = kv.second;
        const SweepTransitionAnalysis *analysis = transition.transition;

        int numOutside = 0;
        int maxOutsideDp = _locatedAtRight
            ? analysis->dataDeltas().maxDpOffset()
            : analysis->dataDeltas().minDpOffset();
        for (const DataDelta& dd : analysis->dataDeltas()) {
            if (dd.dpOffset() != 0) {
                bool insideSweep = (dd.dpOffset() < 0) == _locatedAtRight;
                int sgn = sign(dd.delta());
                if (insideSweep) {
                    // A transition may change values inside the sweep, but only under certain
                    // conditions. Check if these hold.
                    int sweepDeltaChange = _parent->combinedSweepValueChange();

                    if (sweepDeltaChange == 0) {
                        // The loop does not make any changes.
                        if (_insideSweepTransitionDeltaSign != 0 &&
                            sgn != sign(_insideSweepTransitionDeltaSign)) {
                            // Changes are in opposite directions. Do not allow this (for now?).
                            // There could be strange interactions.
                            return failed(*this);
                        }
                        // Change are in the same direction. That's okay.
                        _insideSweepTransitionDeltaSign = sgn;
                    } else if (sgn == sign(sweepDeltaChange)) {
                        // The transition amplifies the changes made by the sweeps. That's okay.
                    } else if (_parent->combinedSweepValueChangeType()
                               == SweepValueChangeType::UNIFORM_CHANGE
                               && abs(dd.delta()) <= abs(sweepDeltaChange)) {
                        // The transition dampens changes made by the sweep, but does not change
                        // the direction of the delta. That's okay.
                    } else {
                        // All other changes could (eventually) cause the sweep hang to be aborted
                        return failed(*this);
                    }
                } else {
                    switch (_sweepEndType) {
                        case SweepEndType::FIXED_POINT_CONSTANT_VALUE:
                        case SweepEndType::FIXED_POINT_MULTIPLE_VALUES:
                        case SweepEndType::FIXED_POINT_INCREASING_VALUE:
                        case SweepEndType::FIXED_POINT_DECREASING_VALUE: {
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
                            if (!loop()->isExitValue(dd.delta())) {
                                return failed(*this);
                            }
                            break;
                        case SweepEndType::STEADY_GROWTH:
                            if (dd.dpOffset() != maxOutsideDp && loop()->isExitValue(dd.delta())) {
                                // The next sweep does not pass this value, which it should if the
                                // sequence is steadily growing
                                return failed(*this);
                            }
                            ++numOutside;
                            break;
                        case SweepEndType::IRREGULAR_GROWTH:
                            // Do not support this (yet?)
                            return failed(*this);
                        case SweepEndType::UNKNOWN:
                            assert(false);
                    }
                }
            }
        }

        if (_sweepEndType == SweepEndType::STEADY_GROWTH && numOutside != abs(maxOutsideDp)) {
            // Multiple values are added beyond the DP outside the sequence, but they do not form
            // a continuous region. I.e. one or more zeroes are introduces in the sequence.
            return failed(*this);
        }
    }

    return true;
}

bool SweepTransitionGroup::allOutsideDeltasMoveAwayFromZero(DataPointer dp, const Data& data) const {
    for (const DataDelta &dd : _outsideDeltas) {
        if (sign(dd.delta()) != sign(data.valueAt(dp, dd.dpOffset()))) {
            return false;
        }
    }

    return true;
}

Trilian SweepTransitionGroup::proofHang(DataPointer dp, const Data& data) {
    switch (_sweepEndType) {
        case SweepEndType::FIXED_POINT_INCREASING_VALUE:
            if (*dp < 0 || !allOutsideDeltasMoveAwayFromZero(dp, data)) {
                return Trilian::MAYBE;
            }
            break;
        case SweepEndType::FIXED_POINT_DECREASING_VALUE:
            if (*dp > 0 || !allOutsideDeltasMoveAwayFromZero(dp, data)) {
                return Trilian::MAYBE;
            }
            break;
        case SweepEndType::FIXED_POINT_CONSTANT_VALUE:
        case SweepEndType::FIXED_POINT_MULTIPLE_VALUES:
            if (!allOutsideDeltasMoveAwayFromZero(dp, data)) {
                return Trilian::MAYBE;
            }
            break;
        case SweepEndType::FIXED_GROWING: {
            int delta = locatedAtRight() ? 1 : -1;
            // Skip all appendix values
            while (true) {
                int val = data.valueAt(dp, delta);
                if (val == 0 || !loop()->isExitValue(val)) {
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

        case SweepEndType::UNKNOWN:
            assert(false);
    }
    return Trilian::YES;
}

std::ostream &operator<<(std::ostream &os, const SweepTransitionGroup &group) {
    for (auto kv : group._loops) {
        int sequenceIndex = kv.first;
        const SweepLoopAnalysis *loopAnalysis = kv.second;

        os << "Loop #" << sequenceIndex << std::endl;
        os << *loopAnalysis;
    }

    auto iter = group._transitions.begin();
    while (iter != group._transitions.end()) {
        SweepLoopExit loopExit = iter->first;
        SweepTransition transition = iter->second;
        os << "  Exit from Loop" << loopExit.loop->loopRunBlock()->getSequenceIndex();
        os << "@" << loopExit.exitInstructionIndex;
        os << ": " << *(transition.transition);
        os << " => Loop#" << transition.nextLoop->loopRunBlock()->getSequenceIndex() << std::endl;
        ++iter;
    }

    if (group.endType() != SweepEndType::UNKNOWN) {
        os << "Type = " << group.endType();
    }

    return os;
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
    SweepTransitionGroup *group = _transitionGroups;
    auto loop0 = group[0].loop(), loop1 = group[1].loop();

    auto type0 = loop0->sweepValueChangeType(), type1 = loop1->sweepValueChangeType();
    if (type0 == SweepValueChangeType::NO_CHANGE) {
        _sweepValueChangeType = type1;
        _sweepValueChange = loop1->sweepValueChange();
    } else if (type1 == SweepValueChangeType::NO_CHANGE) {
        _sweepValueChangeType = type0;
        _sweepValueChange = loop0->sweepValueChange();
    } else if (
        type0 == SweepValueChangeType::UNIFORM_CHANGE &&
        type1 == SweepValueChangeType::UNIFORM_CHANGE
    ) {
        _sweepValueChange = loop0->sweepValueChange() + loop1->sweepValueChange();
        _sweepValueChangeType = (_sweepValueChange
                                 ? SweepValueChangeType::UNIFORM_CHANGE
                                 : SweepValueChangeType::NO_CHANGE);
    } else if (
        type0 != SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES &&
        type1 != SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES &&
        sign(loop0->sweepValueChange()) == sign(loop1->sweepValueChange())
    ) {
        _sweepValueChangeType = SweepValueChangeType::MULTIPLE_ALIGNED_CHANGES;
        // Only the sign matters, so addition is not needed, but makes it nicely symmetrical
        _sweepValueChange = loop0->sweepValueChange() + loop1->sweepValueChange();
    } else {
        // Both loops make opposite changes that do not fully cancel out. We cannot (yet?) detect
        // hangs of this type.
        return failed(_executor);
    }

    if (
        (loop0->requiresFixedInput() || loop1->requiresFixedInput()) &&
        _sweepValueChangeType != SweepValueChangeType::NO_CHANGE
    ) {
        // The changes of both loops, if any, should cancel each other out. They don't
        return failed(_executor);
    }

    return true;
}

bool SweepHangDetector::analyzeLoops() {
    const RunSummary& runSummary = _executor.getRunSummary();
    SweepTransitionGroup *group = _transitionGroups;

    // Assume that the loop which just finished is one of the sweep loops
    if (runSummary.getLoopIteration() < MIN_ITERATIONS_LAST_SWEEP_LOOP) {
        return failed(_executor);
    }
    int runBlockIndexLoop1 = runSummary.getNumRunBlocks() - 1;
    if (!group[1].analyzeLoop(&_loopAnalysisPool[1], runBlockIndexLoop1, _executor)) {
        return false;
    }

    int runBlockIndexTransition0 = findPrecedingTransitionStart(runBlockIndexLoop1);
    if (runBlockIndexTransition0 == 0) {
        // There is no sweep loop preceding the transition
        return failed(_executor);
    }
    int runBlockIndexLoop0 = runBlockIndexTransition0 - 1;
    if (!group[0].analyzeLoop(&_loopAnalysisPool[0], runBlockIndexLoop0, _executor)) {
        return false;
    }

    // Both loops should move in opposite directions
    if (group[0].locatedAtRight() == group[1].locatedAtRight()) {
        return failed(_executor);
    }

    if (!determineCombinedSweepValueChange()) {
        return false;
    }

    return true;
}

bool SweepHangDetector::analyzeTransitions() {
    const RunSummary& runSummary = _executor.getRunSummary();
    const RunSummary& metaRunSummary = _executor.getMetaRunSummary();
    int metaLoopStartIndex = metaRunSummary.getLastRunBlock()->getStartIndex();
    int prevLoopIndex = runSummary.getNumRunBlocks() - 1;
    const SweepLoopAnalysis *prevLoopAnalysis = _transitionGroups[1].loop();
    int numSweeps = 0, numUniqueTransitions = 0, numUniqueLoops = 2;

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
        const RunBlock* existingLoopBlock = tg.loop()->loopRunBlock();
        if (loopBlock->getSequenceIndex() != existingLoopBlock->getSequenceIndex() &&
            !runSummary.areLoopsRotationEqual(loopBlock, existingLoopBlock)
        ) {
            // Sequence does not follow expected sweep pattern anymore
            break;
        }

        const SweepLoopAnalysis* sla = tg.analysisForLoop(loopBlock);
        if (sla == nullptr) {
            // This loop was entered differently than the one(s) encountered already. Analyse it
            assert(numUniqueLoops < MAX_SWEEP_LOOP_ANALYSIS);
            SweepLoopAnalysis* newAnalysis = &_loopAnalysisPool[numUniqueLoops++];

            assert(tg.analyzeLoop(newAnalysis, loopIndex, _executor));
            sla = newAnalysis;
        }

        int loopLen = runSummary.getRunBlockLength(loopIndex);
        int exitInstruction = (loopLen - 1) % loopBlock->getLoopPeriod();
        SweepLoopExit loopExit(sla, exitInstruction);
        const SweepTransition* st = tg.transitionForExit(loopExit);
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
                return failed(_executor);
            }

            SweepTransition newTransition(sta, prevLoopAnalysis);
            tg.addTransitionForExit(loopExit, newTransition);
        }

        prevLoopIndex = loopIndex;
        prevLoopAnalysis = sla;
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
        if (val == 0 || !group.loop()->isExitValue(val)) {
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
    int sweepDeltaSign = sign(_sweepValueChange);

    DataPointer dpEnd = (delta > 0) ? data.getMaxDataP() : data.getMinDataP();
    const SweepLoopAnalysis *loop0 = _transitionGroups[0].loop();
    const SweepLoopAnalysis *loop1 = _transitionGroups[1].loop();

    // DP is at one side of the sweep. Find the other end of the sweep.
    dp += delta;
    while (*dp) {
        if (loop0->isExitValue(*dp) || loop1->isExitValue(*dp)) {
            // Found end of sweep at other end
            break;
        }

        if (sweepDeltaSign && sweepDeltaSign != sign(*dp)) {
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
