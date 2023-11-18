//
//  SweepTransitionGroup.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 13/11/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "SweepTransitionGroup.h"

#include "ExecutionState.h"
#include "SweepHangDetector.h"

//#define SWEEP_DEBUG_TRACE

int numTransitionGroupFailures = 0;

bool transitionGroupFailure(const ExecutionState& execution) {
//    execution.dumpExecutionState();
    numTransitionGroupFailures++;
    return false;
}

bool transitionGroupFailure(const SweepTransitionGroup& tg) {
//    std::cout << tg << std::endl;
    numTransitionGroupFailures++;
    return false;
}

SweepEndType unsupportedSweepEndType(const SweepTransitionGroup& tg) {
    std::cout << tg << std::endl;
    numTransitionGroupFailures++;
    return SweepEndType::UNSUPPORTED;
}

std::ostream &operator<<(std::ostream &os, SweepEndType endType) {
    switch (endType) {
        case SweepEndType::STEADY_GROWTH: os << "Steady growth"; break;
        case SweepEndType::IRREGULAR_GROWTH: os << "Irregular growth"; break;
        case SweepEndType::FIXED_POINT_CONSTANT_VALUE: os << "Fixed point, constant value"; break;
        case SweepEndType::FIXED_POINT_MULTIPLE_VALUES: os << "Fixed point, multiple values"; break;
        case SweepEndType::FIXED_POINT_INCREASING_VALUE: os << "Fixed point, increasing value"; break;
        case SweepEndType::FIXED_POINT_DECREASING_VALUE: os << "Fixed point, decreasing value"; break;
        case SweepEndType::FIXED_APERIODIC_APPENDIX: os << "Aperiodic appendix"; break;
        case SweepEndType::UNSUPPORTED: os << "Unsupported"; break;
        case SweepEndType::UNKNOWN: break;
    }

    return os;
}

bool SweepLoopAnalysis::isExitValue(int value) const {
    return _requiresFixedInput ? value != _requiredInput : _exitMap.find(value) != _exitMap.end();
}

int SweepLoopAnalysis::numberOfExitsForValue(int value) const {
    return (int)_exitMap.count(value);
}

void SweepLoopAnalysis::collectInsweepDeltasAfterExit(int exitInstruction,
                                                      DataDeltas &dataDeltas) const {
    dataDeltas.clear();

    // Fully bootstrap the loop
    int maxIteration = numBootstrapCycles() + 1;
    // Set it such that on exit, dpDelta is zero
    int dpDelta = -(maxIteration * _dpDelta + effectiveResultAt(exitInstruction).dpOffset());
    for (int iteration = 0; iteration <= maxIteration; iteration++) {
        // Execute the last iteration until (inclusive) the exit instruction
        for (int instruction = 0; instruction < loopSize(); instruction++) {
            const ProgramBlock* pb = _programBlocks[instruction];

            if (pb->isDelta()) {
                dataDeltas.updateDelta(dpDelta, pb->getInstructionAmount());
            } else {
                dpDelta += pb->getInstructionAmount();
            }

            if (instruction == exitInstruction && iteration == maxIteration) {
                assert(dpDelta == 0);
                int dpDeltaStartIteration = dpDelta - effectiveResultAt(exitInstruction).dpOffset();
                int dpMin = dpDeltaStartIteration + minDp();
                int dpMax = dpDeltaStartIteration + maxDp();
                auto it = dataDeltas.begin();
                while (it != dataDeltas.end()) {
                    DataDelta dd = *it;
                    bool insideSweep = sign(dpDelta - dd.dpOffset()) == sign(dataPointerDelta());
                    bool isBootstrapResidu = dd.dpOffset() < dpMin || dd.dpOffset() > dpMax;
                    if (!insideSweep || isBootstrapResidu) {
                        it = dataDeltas.erase(it);
                    } else {
                        ++it;
                    }
                }

                return;
            }
        }
    }

    assert(false);
}

// TODO: Replace by exit-aware version?
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
                                         const ExecutionState& execution) {
    _loopRunBlock = runBlock;

    const RunHistory& runHistory = execution.getRunHistory();

    if (!analyzeLoop(&runHistory[_loopRunBlock->getStartIndex()], _loopRunBlock->getLoopPeriod())) {
        return transitionGroupFailure(execution);
    }

    return true;
}

bool SweepLoopAnalysis::finishAnalysis() {
    if (!LoopAnalysis::finishAnalysis()) {
        return false;
    }

    if (dataPointerDelta() == 0) {
        // A sweep loop cannot be stationary
        return false;
    }

    _sweepValueChanges.clear();
    _sweepValueChange = 0;
    for (auto dd : squashedDataDeltas()) {
        int delta = dd.delta();
        _sweepValueChanges.insert(delta);
        _sweepValueChange = delta;
    }

    if (squashedDataDeltas().size() < abs(dataPointerDelta())) {
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
                _requiredInput = exit(i).exitCondition.value();
            }
        }
    }

    return true;
}

std::ostream &operator<<(std::ostream &os, const SweepLoopAnalysis& sla) {
    os << (const LoopAnalysis&)sla;
    os << "Exit values: ";

    if (sla._requiresFixedInput) {
        os << "not " << sla._requiredInput;
    } else {
        bool isFirst = true;
        for (auto pair : sla._exitMap) {
            if (isFirst) {
                isFirst = false;
            } else {
                os << ", ";
            }
            os << pair.first << "@" << pair.second;
        }
    }
    os << std::endl;

    return os;
}

void SweepTransitionAnalysis::analyzeSweepTransition(int startIndex, int endIndex,
                                                     const ExecutionState& execution) {
    const RunHistory& runHistory = execution.getRunHistory();
    const RunSummary& runSummary = execution.getRunSummary();

    // The instructions comprising the transition sequence
    _pbStartIndex = runSummary.runBlockAt(startIndex)->getStartIndex();
    int numProgramBlocks = runSummary.runBlockAt(endIndex)->getStartIndex() - _pbStartIndex;

    analyzeSequence(&runHistory[_pbStartIndex], numProgramBlocks);
}

bool SweepTransitionAnalysis::transitionEquals(int startIndex, int endIndex,
                                               const ExecutionState& execution) const {
    const RunSummary& runSummary = execution.getRunSummary();

    // The instructions comprising the transition sequence
    int pbStart = runSummary.runBlockAt(startIndex)->getStartIndex();
    int numProgramBlocks = runSummary.runBlockAt(endIndex)->getStartIndex() - pbStart;

    if (numProgramBlocks != sequenceSize()) {
        return false;
    }

    auto pb1 = &execution.getRunHistory()[_pbStartIndex];
    auto end = pb1 + numProgramBlocks;
    auto pb2 = &execution.getRunHistory()[pbStart];
    while (pb1 != end) {
        if (*pb1++ != *pb2++) return false;
    }

    return true;
}

void SweepTransitionAnalysis::dump() const {
    std::cout << *this << std::endl;
}

void SweepEndTypeAnalysis::addInsweepDeltasAfterTransition(SweepTransition st,
                                                           DataDeltas &dataDeltas) const {
//    std::cout << "After incoming loop: " << dataDeltas << std::endl;

    // Add any changes made by the transition
    for (auto dd : st.transition->dataDeltas()) {
        if (dd.dpOffset() != 0 && (dd.dpOffset() < 0) == _group.locatedAtRight()) {
            dataDeltas.updateDelta(dd.dpOffset(), dd.delta());
        }
    }
//    std::cout << "After transition: " << dataDeltas << std::endl;

    // Finally, apply changes made by the outgoing loop
    int dpLoopOffset = 0;
    auto outgoingLoop = _group._outgoingLoop;
    if (st.nextLoopStartIndex > 0) {
        // Take into account that the loop does not start at the first instruction
        dpLoopOffset = -outgoingLoop->effectiveResultAt(st.nextLoopStartIndex - 1).dpOffset();
    }
    for (int dpOffset = _group.locatedAtRight() ? dataDeltas.minDpOffset() : 1,
             max = _group.locatedAtRight() ? -1 : dataDeltas.maxDpOffset();
         dpOffset <= max;
         dpOffset++
    ) {
        dataDeltas.updateDelta(dpOffset + st.transition->dataPointerDelta(),
                               outgoingLoop->deltaAt(dpOffset + dpLoopOffset));
    }
//    std::cout << "After outgoing loop: " << dataDeltas << std::endl;
}

DataDeltas addInSweepDeltasForExitDataDeltas;
void SweepEndTypeAnalysis::addInSweepDeltasForExit(std::set<int> &deltas,
                                                   int exitInstruction) const {
    auto &dataDeltas = addInSweepDeltasForExitDataDeltas;

    // Determine what all inside-sweep deltas can be when this exit is taken
    auto beg = _group._transitionMap.lower_bound(exitInstruction);
    auto end = _group._transitionMap.upper_bound(exitInstruction);

    while (beg != end) {
        auto &sweepTransition = beg->second;

        _group._incomingLoop->collectInsweepDeltasAfterExit(exitInstruction, dataDeltas);
        addInsweepDeltasAfterTransition(sweepTransition, dataDeltas);

        for (DataDelta dd : dataDeltas) {
            deltas.insert(dd.delta());
        }

        ++beg;
    }
}

void SweepEndTypeAnalysis::collectExitRelatedInsweepDeltas(std::set<int> &deltas) const {
    deltas.clear();

    auto incomingLoop = _group.incomingLoop();
    for (int instructionIndex = incomingLoop->loopSize(); --instructionIndex >= 0; ) {
        const LoopExit& loopExit = incomingLoop->exit(instructionIndex);

        if (loopExit.exitWindow == ExitWindow::ANYTIME) {
            addInSweepDeltasForExit(deltas, instructionIndex);
        }
    }
}

void SweepEndTypeAnalysis::collectSweepDeltas(std::set<int> &deltas) const {
    deltas.clear();

    if (_group._sweepValueChangeType != SweepValueChangeType::NO_CHANGE) {
        if (_group._incomingLoop->sweepValueChangeType() != SweepValueChangeType::NO_CHANGE) {
            for (auto dd : _group._incomingLoop->squashedDataDeltas()) {
                deltas.insert(dd.delta());
            }
        }
        if (_group._outgoingLoop->sweepValueChangeType() != SweepValueChangeType::NO_CHANGE) {
            for (auto dd : _group._outgoingLoop->squashedDataDeltas()) {
                deltas.insert(dd.delta());
            }
        }
    }
}

bool SweepEndTypeAnalysis::valueCanBeChangedToExitByDeltas(int value, std::set<int> &deltas) const {
    for (auto kv : _group._transitionMap) {
        const LoopExit &loopExit = _group._incomingLoop->exit(kv.first);

        if (loopExit.exitWindow == ExitWindow::ANYTIME) {
            int exitValue = loopExit.exitCondition.value();

            if (deltasCanSumTo(deltas, exitValue - value)) {
                return true;
            }
        }
    }

    return false;
}

int SweepEndTypeAnalysis::deltaAfterExit(const LoopExit& loopExit,
                                         const SweepTransition& trans) const {
    const SweepTransitionAnalysis *sta = trans.transition;

    // Determine delta after exit
    int delta = sta->dataDeltas().deltaAt(0);

    // Add any changes made by the outgoing loop
    auto outgoingLoop = _group.outgoingLoop();
    int dpOffset = -sta->dataPointerDelta();

    if (trans.nextLoopStartIndex > 0) {
        // Take into account that the loop does not start at the first instruction
        delta += outgoingLoop->deltaAtOnNonStandardEntry(dpOffset, trans.nextLoopStartIndex);
    } else {
        delta += outgoingLoop->deltaAt(dpOffset);
    }

    return delta;
}

std::set<int> analyseExitsSet1, analyseExitsSet2;
bool SweepEndTypeAnalysisZeroExits::analyseExits() {
    auto &insweepExitDeltas = analyseExitsSet1;
    auto &sweepDeltas = analyseExitsSet2;
    collectExitRelatedInsweepDeltas(insweepExitDeltas);
    collectSweepDeltas(sweepDeltas);

    _exitValueChanges = false;
    _exitToExit = _exitToLimbo = _exitToSweepBody = 0;
    _limboToExitBySweep = _limboToExitByLoopExit = _limboToSweepBody = 0;

    for (auto kv : _group._transitionMap) {
        const LoopExit &loopExit = _group._incomingLoop->exit(kv.first);
        const SweepTransition &trans = kv.second;

        int delta = deltaAfterExit(loopExit, trans);

        if (delta != loopExit.exitCondition.value()) {
            // The initial value that caused the exit is changed
            _exitValueChanges = true;
        }

        int exitCount = _group._incomingLoop->numberOfExitsForValue(delta);

        if (exitCount > 0) {
            // The value that caused the loop to exit and continue at the given transition, is
            // converted (by the transition) to another value that causes a loop exit (in a
            // later sweep).
            ++_exitToExit;

            int numTransitions = _group.numberOfTransitionsForExitValue(delta);
            if (numTransitions < exitCount) {
                // Not all possible exits are (yet) followed by a transition that continues the
                // sweep. We can therefore not yet conclude this is a hang.
                return transitionGroupFailure(_group);
            }
        } else {
            // This transition extends the sequence. It results in a value that does not
            // (directly) cause the loop to exit in a future sweep. Check if it could possibly
            // be changed to an exit. If so, it is in limbo. Otherwise it is part of the sweep
            // body.

            bool isLimbo = false;
            if (valueCanBeChangedToExitByDeltas(delta, insweepExitDeltas)) {
                ++_limboToExitByLoopExit;
                isLimbo = true;
            }
            if (valueCanBeChangedToExitByDeltas(delta, sweepDeltas)) {
                ++_limboToExitBySweep;
                isLimbo = true;
            } else if (isLimbo &&
                       _group.combinedSweepValueChangeType() != SweepValueChangeType::NO_CHANGE) {
                // Eventually the sweep will.
                ++_limboToSweepBody;
            }

            if (isLimbo) {
                ++_exitToLimbo;
            } else {
                ++_exitToSweepBody;
            }
        }
    }

    return true;
}

SweepEndType SweepEndTypeAnalysisZeroExits::classifySweepEndType() {
    bool exitToNonExit = _exitToLimbo || _exitToSweepBody;

    if (!_exitToExit && _exitToSweepBody && !_exitToLimbo) {
        return SweepEndType::STEADY_GROWTH;
    }
    if (_exitToExit && !exitToNonExit) {
        if (_exitValueChanges) {
            return SweepEndType::FIXED_POINT_MULTIPLE_VALUES;
        } else {
            return SweepEndType::FIXED_POINT_CONSTANT_VALUE;
        }
    }
    if (_exitToExit && _exitToSweepBody && !_exitToLimbo) {
        return SweepEndType::IRREGULAR_GROWTH;
    }

    assert(_exitToLimbo);
    assert(_limboToExitBySweep || _limboToExitByLoopExit);

    if (_group.numberOfTransitionsForExitValue(0) == 0) {
        return SweepEndType::UNKNOWN;
    }

    if (_exitToSweepBody) {
        return SweepEndType::IRREGULAR_GROWTH;
    }

    // Try to identify the a-periodic appendix
    if (_group.numberOfTransitionsForExitValue(0) > 0) {
        if (_group._sweepValueChangeType == SweepValueChangeType::UNIFORM_CHANGE) {
            if (_limboToExitBySweep) {
                return SweepEndType::FIXED_APERIODIC_APPENDIX;
            }
        }
        if (_group._sweepValueChangeType == SweepValueChangeType::NO_CHANGE) {
            if (_limboToExitBySweep) {
                assert(false); // Check if this can actually occur
                return SweepEndType::FIXED_APERIODIC_APPENDIX;
            }
        }
    }

    // Possibly refined elsewhere, with more context
    return SweepEndType::UNKNOWN;
}

SweepEndType SweepEndTypeAnalysisZeroExits::determineSweepEndType() {
    if (!analyseExits()) {
        return SweepEndType::UNSUPPORTED;
    }

    return classifySweepEndType();
}

SweepEndType SweepEndTypeAnalysisNonZeroExits::determineSweepEndType() {
    int numPositiveDeltas = 0;
    int numNegativeDeltas = 0;

    for (auto kv : _group._transitionMap) {
        const LoopExit &loopExit = _group._incomingLoop->exit(kv.first);
        const SweepTransition &trans = kv.second;

        int delta = deltaAfterExit(loopExit, trans);

        // Determine how much the value changes compared to its initial value.
        int totalDelta = delta - loopExit.exitCondition.value();

        if (totalDelta > 0) {
            ++numPositiveDeltas;
        } else if (totalDelta < 0) {
            ++numNegativeDeltas;
        }
    }

    if (numNegativeDeltas > 0 && numPositiveDeltas > 0) {
        // To facilitate analysis, require that deltas are in the same directions.
        return unsupportedSweepEndType(_group);
    }
    if (numPositiveDeltas > 0) {
        return SweepEndType::FIXED_POINT_INCREASING_VALUE;
    } else if (numNegativeDeltas > 0) {
        return SweepEndType::FIXED_POINT_DECREASING_VALUE;
    } else {
        return SweepEndType::FIXED_POINT_CONSTANT_VALUE;
    }
}

std::ostream &operator<<(std::ostream &os, const SweepTransitionAnalysis& sta) {
    os << (const SequenceAnalysis&)sta;

    return os;
}

SweepTransitionGroup::SweepTransitionGroup()
    : _zeroExitEndTypeAnalysis(*this), _nonZeroExitEndTypeAnalysis(*this) {}

void SweepTransitionGroup::clear() {
    _transitionMap.clear();
    _incomingLoop = nullptr;
    _outgoingLoop = nullptr;
    _midSweepTransition = nullptr;
    _sweepEndType = SweepEndType::UNKNOWN;
    _sweepExitDeltas.clear();
}

int SweepTransitionGroup::numberOfTransitionsForExitValue(int value) const {
    int count = 0;

    for (auto kv : _transitionMap) {
        int exitIndex = kv.first;

        if (_incomingLoop->exit(exitIndex).exitCondition.isTrueForValue(value)) {
            ++count;
        }
    }

    return count;
}

bool SweepTransitionGroup::isSweepGrowing() const {
    int sum = 0;

    for (int delta : _sweepExitDeltas) {
        sum += delta;
    }

    return sum != 0;
}

bool SweepTransitionGroup::isSweepGrowthConstant() const {
    auto it = _sweepExitDeltas.cbegin();
    int firstDelta = *it;

    while (++it != _sweepExitDeltas.cend()) {
        if (*it != firstDelta) {
            return false;
        }
    }

    return true;
}

bool SweepTransitionGroup::determineZeroExitSweepEndType() {
    _sweepEndType = _zeroExitEndTypeAnalysis.determineSweepEndType();

    return didDetermineEndType();
}

bool SweepTransitionGroup::determineNonZeroExitSweepEndType() {
    _sweepEndType = _nonZeroExitEndTypeAnalysis.determineSweepEndType();

    return didDetermineEndType();
}

bool SweepTransitionGroup::determineSweepEndType() {
    int numNonZeroExits = 0;
    int numExits = 0;

//    std::cout << *this;

    for (auto kv : _transitionMap) {
        const LoopExit &loopExit = _incomingLoop->exit(kv.first);

        if (loopExit.exitWindow == ExitWindow::ANYTIME) {
            ++numExits;

            if (loopExit.exitCondition.getOperator() != Operator::EQUALS) {
                ++numNonZeroExits;
            }
        }
    }

    if (numNonZeroExits == 0) {
        return determineZeroExitSweepEndType();
    } else if (numNonZeroExits == numExits) {
        return determineNonZeroExitSweepEndType();
    }

    // To facilitate analysis, when one exit is on a non-zero value, require that all are.
    _sweepEndType = SweepEndType::UNSUPPORTED;

    return transitionGroupFailure(*this);
}

bool SweepTransitionGroup::determineCombinedSweepValueChange() {
    auto incomingType = _incomingLoop->sweepValueChangeType();
    auto outgoingType = _outgoingLoop->sweepValueChangeType();

    if (incomingType == SweepValueChangeType::NO_CHANGE) {
        _sweepValueChangeType = outgoingType;
        _sweepValueChange = _outgoingLoop->sweepValueChange();
    } else if (outgoingType == SweepValueChangeType::NO_CHANGE) {
        _sweepValueChangeType = incomingType;
        _sweepValueChange = _incomingLoop->sweepValueChange();
    } else if (
        incomingType == SweepValueChangeType::UNIFORM_CHANGE &&
        outgoingType == SweepValueChangeType::UNIFORM_CHANGE
    ) {
        _sweepValueChange = _incomingLoop->sweepValueChange() + _outgoingLoop->sweepValueChange();
        _sweepValueChangeType = (_sweepValueChange
                                 ? SweepValueChangeType::UNIFORM_CHANGE
                                 : SweepValueChangeType::NO_CHANGE);
    } else if (
        incomingType != SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES &&
        outgoingType != SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES &&
        sign(_incomingLoop->sweepValueChange()) == sign(_outgoingLoop->sweepValueChange())
    ) {
        _sweepValueChangeType = SweepValueChangeType::MULTIPLE_ALIGNED_CHANGES;
        // Only the sign matters, so addition is not needed, but makes it nicely symmetrical
        _sweepValueChange = _incomingLoop->sweepValueChange() + _outgoingLoop->sweepValueChange();
    } else {
        // Both loops make opposite changes that do not fully cancel out. We cannot (yet?) detect
        // hangs of this type.
        return transitionGroupFailure(*this);
    }

    if (
        (_incomingLoop->requiresFixedInput() || _outgoingLoop->requiresFixedInput()) &&
        _sweepValueChangeType != SweepValueChangeType::NO_CHANGE
    ) {
        // The changes of both loops, if any, should cancel each other out. They don't
        return transitionGroupFailure(*this);
    }

    return true;
}

// TODO: Replace with exit-aware version
bool SweepTransitionGroup::canSweepChangeValueTowardsZero(int value) const {
    if (value == 0 || _sweepValueChangeType == SweepValueChangeType::NO_CHANGE) {
        return false;
    }

    if (_incomingLoop->canSweepChangeValueTowardsZero(value) ||
        _outgoingLoop->canSweepChangeValueTowardsZero(value)
    ) {
        return true;
    }

    return false;
}

const SweepTransition* SweepTransitionGroup::findTransitionMatching(
    int exitInstruction, int startIndex, int endIndex, const ExecutionState& execution
) const {
    for (auto beg = _transitionMap.lower_bound(exitInstruction),
              end = _transitionMap.upper_bound(exitInstruction);
         beg != end; ++beg
    ) {
        const SweepTransition &tr = beg->second;
        if (tr.transition->transitionEquals(startIndex, endIndex, execution)) {
            return &tr;
        }
    }

    return nullptr;
}

bool SweepTransitionGroup::hasUniqueTransitions() const {
    for (auto elem : _transitionMap) {
        const SweepTransition &tr = elem.second;
        if (tr.numOccurences == 1) {
            return true;
        }
    }

    return false;
}

bool SweepTransitionGroup::analyzeSweeps() {
    _locatedAtRight = _incomingLoop->dataPointerDelta() > 0;
    if (_locatedAtRight == (_outgoingLoop->dataPointerDelta() > 0)) {
        // The incoming and outgoing sweep should move in opposite directions
        return transitionGroupFailure(*this);
    }

    if (_incomingLoop->sweepValueChangeType() == SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES ||
        _outgoingLoop->sweepValueChangeType() == SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES
    ) {
        // Not (yet?) supported
        return transitionGroupFailure(*this);
    }

    if (!determineCombinedSweepValueChange()) {
        return false;
    }

    return true;
}

bool SweepTransitionGroup::analyzeGroup() {
    if (!determineSweepEndType()) {
        return transitionGroupFailure(*this);
    }

//    std::cout << *this << std::endl;

    _insideSweepTransitionDeltaSign = 0;
    _outsideDeltas.clear();
    for (auto kv : _transitionMap) {
        const SweepTransition transition = kv.second;
        const SweepTransitionAnalysis *analysis = transition.transition;

        for (const DataDelta& dd : analysis->dataDeltas()) {
            if (dd.dpOffset() != 0) {
                bool insideSweep = (dd.dpOffset() < 0) == _locatedAtRight;
                int sgn = sign(dd.delta());
                if (insideSweep) {
                    // A transition may change values inside the sweep, but only under certain
                    // conditions. Check if these hold.

                    if (_sweepValueChange == 0) {
                        // The loop does not make any changes.

                        if (_insideSweepTransitionDeltaSign == 0) {
                            // Okay, first change
                            _insideSweepTransitionDeltaSign = sgn;
                        } else if (sgn == sign(_insideSweepTransitionDeltaSign)) {
                            // Okay, changes are in the same direction.
                        } else if (hangIsMetaPeriodic()) {
                            // Okay, apparently the various changes result in a stable changes
                            // to the sweep body
                        } else {
                            // Changes are in opposite directions. There could be strange
                            // interactions. Do not support this
                            return transitionGroupFailure(*this);
                        }
                    } else if (sgn == sign(_sweepValueChange)) {
                        // The transition amplifies the changes made by the sweeps. That's okay.
                    } else if (_sweepValueChangeType == SweepValueChangeType::UNIFORM_CHANGE
                               && abs(dd.delta()) <= abs(_sweepValueChange)) {
                        // The transition dampens changes made by the sweep, but does not change
                        // the direction of the delta. That's okay.
                    } else if (_sweepEndType == SweepEndType::FIXED_POINT_MULTIPLE_VALUES) {
                        // The delta is (presumably) part of the periodic behavior that cause this
                        // end-point to remained fixed but oscillate at multiple values.
                        // TODO: Verify that delta is part of value(s) that oscillate periodically?
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
                        case SweepEndType::FIXED_APERIODIC_APPENDIX:
                            if (!_incomingLoop->isExitValue(dd.delta())) {
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
                        case SweepEndType::UNSUPPORTED:
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
        case SweepEndType::FIXED_APERIODIC_APPENDIX: {
//            data.dumpWithCursor(dp);
            int delta = locatedAtRight() ? 1 : -1;
            // Skip all values until first zero. Note, this skips all appendix values, but possibly
            // also polution inside the appendix (i.e. values not involved in the binary-like
            // counting, or in other words, values that do not cause a loop exit).
            while (data.valueAt(dp, delta)) {
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
        case SweepEndType::UNSUPPORTED:
            assert(false);
    }
    return Trilian::YES;
}

std::ostream& SweepTransitionGroup::dumpExitDeltas(std::ostream &os) const {
    os << "Sweep exit deltas:";
    for (int delta : _sweepExitDeltas) {
        os << " " << delta;
    }
    os << std::endl;

    return os;
}

std::ostream& SweepTransitionGroup::dump(std::ostream &os) const {
    os << "Incoming Loop: #" << _incomingLoop->loopRunBlock()->getSequenceId() << std::endl;
    os << *_incomingLoop;

    auto iter = _transitionMap.begin();
    while (iter != _transitionMap.end()) {
        int exitIndex = iter->first;
        SweepTransition transition = iter->second;
        os << "  Exit instruction = " << exitIndex;
        os << ", " << *(transition.transition);
        os << ", Start instruction = " << transition.nextLoopStartIndex << std::endl;
        ++iter;
    }

    os << "Outgoing Loop: #" << _outgoingLoop->loopRunBlock()->getSequenceId() << std::endl;
    os << *_outgoingLoop;
    if (_midSweepTransition != nullptr) {
        os << "  Mid-sweep transition: " << *_midSweepTransition << std::endl;
    }

    dumpExitDeltas(os);

    if (_sweepEndType != SweepEndType::UNKNOWN) {
        os << "Type = " << _sweepEndType;
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const SweepTransitionGroup &group) {
    return group.dump(os);
}
