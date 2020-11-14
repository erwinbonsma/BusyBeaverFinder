//
//  SweepTransitionGroup.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 13/11/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "SweepTransitionGroup.h"

#include "ProgramExecutor.h"
#include "SweepHangDetector.h"

//#define SWEEP_DEBUG_TRACE

int numTransitionGroupFailures = 0;

bool transitionGroupFailure(const ProgramExecutor& executor) {
//    executor.dumpExecutionState();
    numTransitionGroupFailures++;
    return false;
}

bool transitionGroupFailure(const SweepTransitionGroup& tg) {
//    std::cout << tg << std::endl;
    numTransitionGroupFailures++;
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
    int maxIteration = numBootstrapCycles() + 1;
    for (int iteration = 0; iteration <= maxIteration; iteration++) {
        // Execute the last iteration until (inclusive) the exit instruction
        for (int instruction = 0; instruction < loopSize(); instruction++) {
            const ProgramBlock *pb = programBlockAt(instruction);

            if (pb->isDelta()) {
                dataDeltas.updateDelta(dpDelta, pb->getInstructionAmount());
            } else {
                dpDelta += pb->getInstructionAmount();
            }

#ifdef SWEEP_DEBUG_TRACE
            if (instruction == exitInstruction) {
                std::cout << "Iteration " << iteration
                    << " dpOffset = " << dpDelta
                    << ": " << dataDeltas << std::endl;
            }
#endif

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

bool SweepLoopAnalysis::canSweepChangeValueTowardsZero(int value) const {
    for (int delta : sweepValueChanges()) {
        if (delta != 0 &&
            sign(delta) == -sign(value) &&
            abs(delta) <= abs(value)
        ) {
            return true;
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
        return transitionGroupFailure(executor);
    }

    _sweepValueChanges.clear();
    _sweepValueChange = 0;
    for (int i = numDataDeltas(); --i >= 0; ) {
        int delta = dataDeltaAt(i).delta();
        _sweepValueChanges.insert(delta);
        _sweepValueChange = delta;
    }

    if (numDataDeltas() < abs(dataPointerDelta())) {
        _sweepValueChanges.insert(0);
    }
    if (_sweepValueChanges.size() == 1) {
        bool containsZero = _sweepValueChanges.find(0) != _sweepValueChanges.end();
        _sweepValueChangeType = (containsZero
                                 ? SweepValueChangeType::NO_CHANGE
                                 : SweepValueChangeType::UNIFORM_CHANGE);
    } else {
        int firstSign = 0;
        _sweepValueChangeType = SweepValueChangeType::MULTIPLE_ALIGNED_CHANGES;
        for (int value : _sweepValueChanges) {
            if (firstSign != 0) {
                if (firstSign == -sign(value)) {
                    _sweepValueChangeType = SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES;
                    break;
                }
            } else {
                firstSign = sign(value);
            }
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
        return transitionGroupFailure(executor);
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

void SweepTransitionGroup::init(const SweepHangDetector *parent,
                                const SweepTransitionGroup *sibling) {
    _parent = parent;
    _sibling = sibling;
}

void SweepTransitionGroup::clear() {
    _transitions.clear();
    _sweepEndType = SweepEndType::UNKNOWN;
}

bool SweepTransitionGroup::analyzeLoop(int runBlockIndex, const ProgramExecutor& executor) {
    const RunBlock *loopRunBlock = executor.getRunSummary().runBlockAt(runBlockIndex);

    if (!_loop.analyzeSweepLoop(loopRunBlock, executor)) {
        return false;
    }

    if (_loop.sweepValueChangeType() == SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES) {
        // Not (yet?) supported
        return transitionGroupFailure(*this);
    }

    _locatedAtRight = _loop.dataPointerDelta() > 0;

    return true;
}

int SweepTransitionGroup::numberOfTransitionsForExitValue(int value) const {
    int count = 0;

    for (auto kv : _transitions) {
        int exitIndex = kv.first;

        if (_loop.exit(exitIndex).exitCondition.isTrueForValue(value)) {
            ++count;
        }
    }

    return count;
}

int SweepTransitionGroup::numberOfExitsForValue(int value) const {
    return _loop.numberOfExitsForValue(value);
}

bool SweepTransitionGroup::hasIndirectExitsForValue(int value, int dpOffset) const {
    // TODO: When needed, extend to take bootstrap-changes of outgoing sweep into account
    // For that reason, passing dpOffset

    auto sweepValueChangeType = _parent->combinedSweepValueChangeType();
    assert(sweepValueChangeType != SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES);

    // Check if the sweep loop changes the value towards zero
    if (_parent->canSweepChangeValueTowardsZero(value)) {
        return true;
    }

    if (sweepValueChangeType == SweepValueChangeType::NO_CHANGE) {
        // Although the combined sweep does not result in value changes, check if loop exits
        // still can change this value into one that causes an exit in a later sweep.

        if (_loop.hasIndirectExitsForValue(value)) {
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

#ifdef SWEEP_DEBUG_TRACE
    std::cout << *this;
#endif

    for (auto kv : _transitions) {
        const LoopExit &loopExit = _loop.exit(kv.first);
        const SweepTransition &trans = kv.second;
        const SweepTransitionAnalysis *sta = trans.transition;

        // Determine delta after exit. For zero-exits, this is the final value
        int delta = sta->dataDeltas().deltaAt(0);

        // Add any changes made by the outgoing loop
        int dpOffset = -sta->dataPointerDelta();
        if (trans.nextLoopStartIndex > 0) {
            // Take into account that the loop does not start at the first instruction
            dpOffset -= _sibling->loop().effectiveResultAt(trans.nextLoopStartIndex - 1).dpOffset();
        }
        delta += _sibling->loop().deltaAt(dpOffset);

        // Determine how much the value changes compared to its initial value.
        int totalDelta = delta - loopExit.exitCondition.value();

        if (totalDelta > 0) {
            ++numPositiveDeltas;
        } else if (totalDelta < 0) {
            ++numNegativeDeltas;
        }

        bool exitsOnZero = loopExit.exitCondition.getOperator() == Operator::EQUALS;
        if (exitsOnZero) {
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
                    return transitionGroupFailure(*this);
                }
            } else {
                // This transition extends the sequence. It results in a value that does not
                // (directly) cause the loop to exit in a future sweep.

                exitToNonExit = true;

                if (hasIndirectExitsForValue(delta, sta->dataPointerDelta())) {
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
            return transitionGroupFailure(*this);
        }
        if (numNegativeDeltas > 0 && numPositiveDeltas > 0) {
            // To facilitate analysis, require that deltas are not in opposite directions.
            // TODO: Extend analysis when needed
            return transitionGroupFailure(*this);
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
            return transitionGroupFailure(*this);
        } else {
            // Unsupported sweep end type
            return transitionGroupFailure(*this);
        }
    }

    return true;
}

bool SweepTransitionGroup::analyzeGroup() {
    if (!determineSweepEndType()) {
        return false;
    }

//    std::cout << *this << std::endl;

    _insideSweepTransitionDeltaSign = 0;
    _outsideDeltas.clear();
    for (auto kv : _transitions) {
        const SweepTransition transition = kv.second;
        const SweepTransitionAnalysis *analysis = transition.transition;

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
                            return transitionGroupFailure(*this);
                        }
                        // Changes are in the same direction. That's okay.
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
                        return transitionGroupFailure(*this);
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
                                return transitionGroupFailure(*this);
                            }

                            break;
                        }
                        case SweepEndType::FIXED_GROWING:
                            if (!_loop.isExitValue(dd.delta())) {
                                return transitionGroupFailure(*this);
                            }
                            break;
                        case SweepEndType::STEADY_GROWTH:
                            // No additional checks
                            break;
                        case SweepEndType::IRREGULAR_GROWTH:
                            // Do not support this (yet?)
                            return transitionGroupFailure(*this);
                        case SweepEndType::UNKNOWN:
                            assert(false);
                    }
                }
            }
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

bool SweepTransitionGroup::onlyZeroesAhead(DataPointer dp, const Data& data) const {
    if (!data.onlyZerosAhead(dp, locatedAtRight())) {
        return transitionGroupFailure(*this);
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
            assert(false); // Not yet used. TODO: Move elsewhere
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
            if (!onlyZeroesAhead(dp, data)) {
                return Trilian::MAYBE;
            }
            break;

        case SweepEndType::UNKNOWN:
            assert(false);
    }
    return Trilian::YES;
}

std::ostream &operator<<(std::ostream &os, const SweepTransitionGroup &group) {
    os << group._loop;

    auto iter = group._transitions.begin();
    while (iter != group._transitions.end()) {
        int exitIndex = iter->first;
        SweepTransition transition = iter->second;
        os << "  Exit instruction = " << exitIndex;
        os << ", " << *(transition.transition);
        os << ", Start instruction = " << transition.nextLoopStartIndex << std::endl;
        ++iter;
    }

    if (group.endType() != SweepEndType::UNKNOWN) {
        os << "Type = " << group.endType();
    }

    return os;
}

bool PeriodicSweepTransitionGroup::onlyZeroesAhead(DataPointer dp, const Data& data) const {
    int maxOvershoot = 0;
    for (auto kv : _firstTransition.transition->preConditions()) {
        int dpOffset = kv.first;
        if (dpOffset != 0 && locatedAtRight() == (dpOffset > 0)) {
            PreCondition preCondition = kv.second;

            if (!preCondition.holdsForValue(data.valueAt(dp, dpOffset))) {
                return transitionGroupFailure(*this);
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
