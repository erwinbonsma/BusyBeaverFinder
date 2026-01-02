//
//  SweepHangChecker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 16/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "SweepHangChecker.h"

#include "Utils.h"

bool SweepHangChecker::SweepLoop::analyzeCombinedEffect(const SweepHangChecker& checker,
                                                        const ExecutionState &executionState) {
    int loopSize = checker._metaLoopAnalysis->loopSize();

    // Determine the (maximum) size of the data deltas that represent how the data changes when
    // each sweep loop traversed the data once. It is the LCM of the (absolute) DP deltas of each
    // loop.
    //
    // E.g. if one sweep loop moves DP two spots each iteration with data deltas [1, 2] and the
    // other sweep loop moves DP three spots with data deltas [0, 3, 6] the resulting deltas are
    // [1, 5, 7, 2, 4, 8]
    _deltaRange = 1;
    for (int i = 0; i < loopSize; ++i) {
        auto &loc = checker.locationInSweep(i);
        if (loc.isSweepLoop() && loc.isAt(_location)) {
            auto &loopBehavior = checker.loopBehavior(i);
            _deltaRange = lcm(_deltaRange, abs(loopBehavior.loopAnalysis()->dataPointerDelta()));
        }
    }

    _analysis.analyzeMultiSequenceStart();

    int dpMin = (_location == LocationInSweep::LEFT) ? 0 : 1 - _deltaRange;
    int dpMax = (_location == LocationInSweep::LEFT) ? _deltaRange - 1 : 0;
    auto visitor = [this, dpMin, dpMax, &checker](const SweepLoopVisitState& vs) {
        auto &loc = checker.locationInSweep(vs.loopPart.seqIndex());
        if (loc.isSweepLoop() && loc.isAt(this->_location)) {
            return checker.addContributionOfSweepLoopPass(vs, this->_analysis, dpMin, dpMax);
        }
        return true;
    };

    auto dpOffset = checker.visitSweepLoopParts(visitor, executionState, _outgoingLoopSeqIndex);
    if (!dpOffset) {
        return false;
    }

    _analysis.analyzeMultiSequenceEnd(0);

    // Copy deltas within range (ignore deltas outside range)
    _sweepDeltas.clear();
    for (auto& dd : _analysis.dataDeltas()) {
        if (dd.dpOffset() >= dpMin && dd.dpOffset() <= dpMax) {
            _sweepDeltas.addDelta(dd.dpOffset(), dd.delta());
        }
    }

    // Only consider exits inside the loop range.
    _analysis.disableExits([dpMin, dpMax](LoopExit& exit) {
        int dpOffset = exit.exitCondition.dpOffset();
        return dpOffset < dpMin || dpOffset > dpMax;
    });

    // The exit check for this combined analysis requires that the data values that are checked
    // for loop exits (in continuesForever) have not been modified by previous iterations of the
    // sweep loop (in the current sweep). Furthermore, the sweep loop needs to be fully
    // bootstrapped. Set _outgoingLoopNumBootstrapCycles to skip an initial range of data values.
    int firstLoopSize = _analysis.subSequenceLen(0);
    auto outgoingLoopAnalysis = checker._metaLoopAnalysis->loopAnalysis(_outgoingLoopSeqIndex);
    _outgoingLoopNumBootstrapCycles = std::max(firstLoopSize / outgoingLoopAnalysis->loopSize() - 1,
                                               outgoingLoopAnalysis->numBootstrapCycles());

    return true;
}

bool SweepHangChecker::SweepLoop::analyze(const SweepHangChecker& checker,
                                          const ExecutionState& executionState) {
    _outgoingLoopSeqIndex = checker.findSweepLoop([this](const Location& loc) {
        return loc.start == this->_location;
    });

    return analyzeCombinedEffect(checker, executionState);
}

LoopExitOccurence SweepHangChecker::SweepLoop::continuesForever(const ExecutionState& es) const {
    auto &data = es.getData();

    int dpDir = _location == LocationInSweep::LEFT ? 1 : -1;
    int dpOffset = dpDir * _deltaRange * _outgoingLoopNumBootstrapCycles;

    // The amount of instructions from the first loop that are included in the analysis. More than
    // one iteration may be included. This is used to check if the exit that is found is the exit
    // for the outgoing sweep loop that is currently being executed.
    int firstLoopSize = _analysis.subSequenceLen(0);

    // We are looking for the exit of the currently executing sweep loop, while checking if any
    // parts of the sweep body before we get there eventually cause a mid-sweep exit.
    while (true) {
        auto exitResult = _analysis.stationaryLoopExits(data, dpOffset);
        if (
            exitResult
            && (
                // We found a position that eventually causes a mid-sweep exit
                exitResult.value().iteration > 0 ||
                // We found the exit of the current sweep loop
                exitResult.value().instructionIndex < firstLoopSize
            )
        ) {
            return exitResult.value();
        }

        dpOffset += dpDir * _deltaRange;
    }
}

bool SweepHangChecker::TransitionGroup::addSequenceInstructions(const SweepLoopVisitState& vs) {
    auto &runSummary = vs.executionState.getRunSummary();
    auto &part = vs.loopPart;
    auto runBlock = runSummary.runBlockAt(part.rbIndex());
    int pbIndex = runBlock->getStartIndex();

    _analysis.analyzeMultiSequence(&vs.executionState.getRunHistory().at(pbIndex),
                                   runSummary.getRunBlockLength(part.rbIndex()),
                                   part.dpOffset());

    return true;
}

bool SweepHangChecker::TransitionGroup::addLoopInstructions(const SweepLoopVisitState& vs,
                                                            bool incoming) {
    auto &runHistory = vs.executionState.getRunHistory();
    auto &runSummary = vs.executionState.getRunSummary();
    auto &part = vs.loopPart;
    auto &loopBehavior = vs.checker.loopBehavior(part.seqIndex());
    auto la = loopBehavior.loopAnalysis();
    int numIter = la->numBootstrapCycles();
    int len = numIter * la->loopSize();

    if (len > runSummary.getRunBlockLength(part.rbIndex())) {
        return false;
    }

    // Range:
    // - DP-range of all sequences (which can be inside the sequence)
    // - Location LEFT/RIGHT:
    //   - DP-range of connected sweep loop (to DP = 0, i.e. first loop exit)
    // - Location MID:
    //   - For terminating loops. DP-range of connected sweep loop
    //   - For traversing loops. Do not modify DP-range but should traverse it fully

    if (incoming) {
        // Incoming loop. Execute last "numIter" full loop iterations + any remaining instructions
        auto &analysis = vs.checker._metaLoopAnalysis;

        int dpEnd = part.dpOffset() + analysis->dpDeltaOfRunBlock(runSummary, part.rbIndex());
        int loopIndex = analysis->loopIndexForSequence(part.seqIndex());
        int remainder = analysis->loopRemainder(loopIndex);
        int dpDeltaLastIter = ((remainder > 0)
                               ? la->effectiveResultAt(remainder - 1).dpOffset() : 0);
        int dpStart = dpEnd - dpDeltaLastIter - numIter * la->dataPointerDelta();
        int pbIndexNext = runSummary.runBlockAt(part.rbIndex() + 1)->getStartIndex();
        len += remainder;

        if (len > runSummary.getRunBlockLength(part.rbIndex())) {
            return false;
        }
        _analysis.analyzeMultiSequence(&runHistory.at(pbIndexNext - len), len, dpStart);
    } else {
        // Outgoing loop. Execute first "numIter" loop iterations
        int pbIndex = runSummary.runBlockAt(part.rbIndex())->getStartIndex();

        _analysis.analyzeMultiSequence(&runHistory.at(pbIndex), len, part.dpOffset());
    }

    return true;
}

bool SweepHangChecker::TransitionGroup::analyzeLoopPartPhase1(const SweepLoopVisitState& vs) {
    //    std::cout << "seqIndex = " << vs.loopPart.seqIndex()
    //    << ", dp = " << vs.loopPart.dpOffset()
    //    << ", rbIndex = " << vs.loopPart.rbIndex()
    //    << std::endl;

    auto &loc = vs.checker.locationInSweep(vs.loopPart.seqIndex());
    if (loc.isAt(_location)) {
        if (loc.isSweepLoop()) {
            return addLoopInstructions(vs, loc.end == _location);
        } else {
            return addSequenceInstructions(vs);
        }
    }

    return true;
}

bool SweepHangChecker::TransitionGroup::analyzeLoopPartPhase2(const SweepLoopVisitState& vs) {
//    auto& part = vs.loopPart;
//    std::cout << "seqIndex = " << part.seqIndex() << ", dp = " << part.dpOffset() << std::endl;

    bool result = true;

    auto &loc = vs.checker.locationInSweep(vs.loopPart.seqIndex());
    if (loc.isSweepLoop()) {
        if (loc.start == _location) {
            vs.checker.addContributionOfSweepLoopStart(vs, _analysis, _minDp, _maxDp);
        } else if (loc.end == _location) {
            result = vs.checker.addContributionOfSweepLoopEnd(vs, _analysis, _minDp, _maxDp);
        } else if (_location == LocationInSweep::MID) {
            vs.checker.addContributionOfSweepLoopPass(vs, _analysis, _minDp, _maxDp);
        }
    } else {
        if (loc.isAt(_location)) {
            addSequenceInstructions(vs);
        }
    }

    return result;
}

bool SweepHangChecker::TransitionGroup::analyzeCombinedEffect(const SweepHangChecker& checker,
                                                              const ExecutionState& state,
                                                              bool forceStationary) {
    const int loopSize = checker._metaLoopAnalysis->loopSize();

    // Start at instruction after incoming loop. This is required for correct analysis, as the
    // analyzed combined effect is used to check if the "transition loop" continues forever after
    // the incoming sweep loop just terminated. It ensures that the DP offset is zero at the start,
    // and that the exit conditions are relative to the data values as they are then.
    const int startSeqIndex = (_incomingLoopSeqIndex + 1) % loopSize;

    SweepVisitOptions options = { .forceStationary = forceStationary };

    // First determine DP bounds
    _analysis.analyzeMultiSequenceStart();

    auto dpOffset = checker.visitSweepLoopParts([this](const SweepLoopVisitState& vs) {
        return this->analyzeLoopPartPhase1(vs);
    }, state, startSeqIndex, options);
    if (!dpOffset) {
        return false;
    }

    _minDp = _analysis.minDp();
    _maxDp = _analysis.maxDp();

    // Next collect all contributions within this range
    _analysis.analyzeMultiSequenceStart();
    dpOffset = checker.visitSweepLoopParts([this](const SweepLoopVisitState& vs) {
        return this->analyzeLoopPartPhase2(vs);
    }, state, startSeqIndex, options);
    if (!dpOffset) {
        return false;
    }
    _analysis.analyzeMultiSequenceEnd(dpOffset.value());

    // Copy deltas within range (ignore deltas outside range)
    _transitionDeltas.clear();
    for (auto& dd : _analysis.dataDeltas()) {
        if (dd.dpOffset() >= _minDp && dd.dpOffset() <= _maxDp) {
            _transitionDeltas.addDelta(dd.dpOffset(), dd.delta());
        }
    }

    _analysis.disableExits([this](const LoopExit& exit) {
        int dpOffset = exit.exitCondition.dpOffset();
        return dpOffset < this->_minDp || dpOffset > this->_maxDp;
    });

    if (_isStationary && _analysis.dataPointerDelta() != 0) {
        return false;
    }

    return true;
}

bool SweepHangChecker::TransitionGroup::analyze(const SweepHangChecker& checker,
                                                const ExecutionState& executionState,
                                                bool forceStationary) {
    _incomingLoopSeqIndex = checker.findSweepLoop([this](const Location& loc) {
        return loc.end == this->_location;
    });
    auto &loopBehavior = checker.loopBehavior(_incomingLoopSeqIndex);
    _isStationary = (forceStationary
                     || _location == LocationInSweep::MID
                     || (_location == LocationInSweep::RIGHT
                         ? loopBehavior.maxDpDelta() == 0
                         : loopBehavior.minDpDelta() == 0));

    return analyzeCombinedEffect(checker, executionState, forceStationary);
}

bool SweepHangChecker::TransitionGroup::continuesForever(const ExecutionState& execState) const {
    if (_isStationary) {
        return !_analysis.stationaryLoopExits(execState.getData(), 0);
    } else {
        return _analysis.allValuesToBeConsumedAreZero(execState.getData());
    }
}

template <class Pred>
int SweepHangChecker::findSweepLoop(Pred p) const {
    int loopSize = _metaLoopAnalysis->loopSize();

    for (int i = 0; i < loopSize; ++i) {
        auto& loc = locationInSweep(i);
        if (loc.isSweepLoop() && p(loc)) {
            // Found sweep loop satisfying the predicate
            return i;
        }
    }

    return -1;
}

// Adds all contributions from a sweep loop pass over the range [minDp, maxDp]
// DP offset is only used to align the traversal of the loop over the sequence. The loop always
// traverses the full range in this analysis.
bool SweepHangChecker::addContributionOfSweepLoopPass(const SweepLoopVisitState vs,
                                                      SequenceAnalysis& multiPartAnalysis,
                                                      int minDp, int maxDp) const {
    assert(&vs.checker == this);

    auto& part = vs.loopPart;
    auto loopAnalysis = loopBehavior(part.seqIndex()).loopAnalysis();;
    int dpDelta = loopAnalysis->dataPointerDelta();
    int dpOffset = part.dpOffset();

    // Determine number of iterations (so that all instructions that fall inside [minDp, maxDp] are
    // included)
    //
    // dpMax(i) = dpOffset + dpMax + i * dpDelta
    // dpMin(i) = dpOffset + dpMin + i * dpDelta
    int startIter, endIter;
    if (dpDelta > 0) {
        // Iteration where loop first enters DP range: dpMax(i) >= 0
        startIter = ceildiv(minDp - dpOffset - loopAnalysis->maxDp(), dpDelta);

        // Iteration where loop fully exited DP range: dpMin(i) >= deltaRange
        endIter = ceildiv(maxDp - dpOffset - loopAnalysis->minDp() + 1, dpDelta);
    } else {
        // dpMin(i) <= deltaRange - 1  =>  i >= div(..., ...)
        startIter = ceildiv(maxDp - dpOffset - loopAnalysis->minDp(), dpDelta);

        // dpMax(i) < 0  =>  dpMax(i) <= -1  =>  i >= div(..., ...)
        endIter = ceildiv(minDp - dpOffset - loopAnalysis->maxDp() - 1, dpDelta);
    }

    // Determine DP start (so that's correctly aligned with the other loops)
    int dpStart = dpOffset + startIter * dpDelta;
    auto& runSummary = vs.executionState.getRunSummary();
    auto& runHistory = vs.executionState.getRunHistory();
    int pbLen = (endIter - startIter) * loopAnalysis->loopSize();
    int rbIndex = part.rbIndex();

    if (runSummary.getRunBlockLength(rbIndex) < pbLen) {
        // The run block in the run summary is not long enough. See if there is an occurance in
        // the run summary that is sufficiently long
        auto result = runSummary.findLoopOfLength(runSummary.runBlockAt(rbIndex)->getSequenceId(),
                                                  pbLen);
        if (!result) {
            return false;
        }
        rbIndex = result.value();
    }
    auto runBlock = runSummary.runBlockAt(rbIndex);
    multiPartAnalysis.analyzeMultiSequence(&runHistory.at(runBlock->getStartIndex()),
                                           pbLen, dpStart);

    return true;
}

void SweepHangChecker::addContributionOfSweepLoopStart(const SweepLoopVisitState vs,
                                                       SequenceAnalysis& multiPartAnalysis,
                                                       int minDp, int maxDp) const {
    assert(&vs.checker == this);

    auto& part = vs.loopPart;
    auto loopAnalysis = loopBehavior(part.seqIndex()).loopAnalysis();
    int dpDelta = loopAnalysis->dataPointerDelta();
    int dpOffset = part.dpOffset();

    int endIter;
    if (dpDelta > 0) {
        // Iteration where loop fully exited DP range: dpMin(i) >= deltaRange
        endIter = ceildiv(maxDp - dpOffset - loopAnalysis->minDp() + 1, dpDelta);
    } else {
        // dpMax(i) < 0  =>  dpMax(i) <= -1  =>  i >= div(..., ...)
        endIter = ceildiv(minDp - dpOffset - loopAnalysis->maxDp() - 1, dpDelta);
    }
    assert(endIter > 0);

    auto runBlock = vs.executionState.getRunSummary().runBlockAt(part.rbIndex());
    auto& runHistory = vs.executionState.getRunHistory();
    multiPartAnalysis.analyzeMultiSequence(&runHistory.at(runBlock->getStartIndex()),
                                           endIter * loopAnalysis->loopSize(), dpOffset);
}

bool SweepHangChecker::addContributionOfSweepLoopEnd(const SweepLoopVisitState vs,
                                                     SequenceAnalysis& multiPartAnalysis,
                                                     int minDp, int maxDp) const {
    auto& analysis = vs.checker._metaLoopAnalysis;
    auto& part = vs.loopPart;
    auto loopAnalysis = loopBehavior(part.seqIndex()).loopAnalysis();
    auto& runSummary = vs.executionState.getRunSummary();

    int dpEnd = part.dpOffset() + analysis->dpDeltaOfRunBlock(runSummary, part.rbIndex());
    int loopIndex = analysis->loopIndexForSequence(part.seqIndex());
    int remainder = analysis->loopRemainder(loopIndex);
    int dpDeltaLastIter = ((remainder > 0)
                           ? loopAnalysis->effectiveResultAt(remainder - 1).dpOffset() : 0);

    int dpDelta = loopAnalysis->dataPointerDelta();
    if (dpDelta == 0) return false;

    int dpOffset = dpEnd - dpDeltaLastIter;
    int startIter;
    if (dpDelta > 0) {
        // Iteration where loop first enters DP range: dpMax(i) >= 0
        startIter = ceildiv(minDp - dpOffset - loopAnalysis->maxDp(), dpDelta);
    } else {
        // dpMin(i) <= deltaRange - 1  =>  i >= div(..., ...)
        startIter = ceildiv(maxDp - dpOffset - loopAnalysis->minDp(), dpDelta);
    }
    assert(startIter < 0);

    int numIter = abs(startIter);
    int dpStart = dpEnd - dpDeltaLastIter - numIter * loopAnalysis->dataPointerDelta();
    int len = numIter * loopAnalysis->loopSize() + remainder;

    int rbIndex = part.rbIndex();
    int sequenceId = runSummary.runBlockAt(rbIndex)->getSequenceId();
    while (runSummary.getRunBlockLength(rbIndex) < len) {
        // There are not enough run blocks in the history. This can for example happen for
        // irregular sweeps. Go forward in time until there is a sequence that is long enough.

        rbIndex += analysis->loopSize();
        if (rbIndex >= runSummary.getNumRunBlocks() - 1) {
            return false;
        }
        if (runSummary.runBlockAt(rbIndex)->getSequenceId() != sequenceId) {
            return false;
        }
    }

    // Analyse the end of the loop run history, as the first instruction of the next block is also
    // analyzed.
    int pbIndexNext = runSummary.runBlockAt(rbIndex + 1)->getStartIndex();
    auto& runHistory = vs.executionState.getRunHistory();
    multiPartAnalysis.analyzeMultiSequence(&runHistory.at(pbIndexNext - len), len, dpStart);

    return true;
}

std::optional<int> SweepHangChecker::visitSweepLoopParts(const SweepLoopVisitor& visitor,
                                                         const ExecutionState& state,
                                                         int startSeqIndex,
                                                         SweepVisitOptions options) const {
    auto &runSummary = state.getRunSummary();
    int loopSize = _metaLoopAnalysis->loopSize();

    struct SweepLoopPart part {
        startSeqIndex, _metaLoopAnalysis->firstRunBlockIndex() + startSeqIndex, options.dpStart
    };
    struct SweepLoopVisitState visitState(*this, state, part);

    for (int i = loopSize * options.numLoopIterations; --i >= 0; ) {
        if (part.rbIndex() >= runSummary.getNumRunBlocks()) {
            // Cannot go into the future
            return {};
        }

        if (i == 0 && options.forceStationary) {
            // Force that analysis ends where it started. This is a hack to ensure transitions for
            // irregular loops are analyzed correctly.
            part.dpOffset() = (options.dpStart
                               - _metaLoopAnalysis->dpDeltaOfRunBlock(runSummary, part.rbIndex()));
        }

        if (!visitor(visitState)) {
            // The visitor aborted its visit
            return {};
        }

        part.dpOffset() += _metaLoopAnalysis->dpDeltaOfRunBlock(runSummary, part.rbIndex());
        part.seqIndex() = (part.seqIndex() + 1) % loopSize;
        part.rbIndex() += 1;
    }

    return part.dpOffset();
}

bool SweepHangChecker::locateSweepLoops() {
    _locationsInSweep.clear();
    _locationsInSweep.insert(_locationsInSweep.end(), _metaLoopAnalysis->loopSize(), {});
    _firstSweepLoopSeqIndex = _metaLoopAnalysis->loopSize();

    _leftSweepLoop.reset();
    _midTransition.reset();

    // Assume there is a mid-sweep transition and collect its incoming and outgoing loops
    _rightSweepLoop.emplace(LocationInSweep::RIGHT);

    // Extract sweep loops. There should be at least two (one in each direction)
    for (auto &behavior : _metaLoopAnalysis->loopBehaviors()) {
        if (behavior.iterationDelta() == 0) {
            // Ignore small constant-size loops.
            continue;
        }

        if (behavior.loopType() != LoopType::ANCHORED_SWEEP &&
            behavior.loopType() != LoopType::DOUBLE_SWEEP) {
            return false;
        }

        // This is a sweep loop
        _firstSweepLoopSeqIndex = std::min(_firstSweepLoopSeqIndex, behavior.sequenceIndex());
        auto &loc = _locationsInSweep[behavior.sequenceIndex()];
        auto analysis = behavior.loopAnalysis();
        int dpDelta = analysis->dataPointerDelta();

        if (behavior.loopType() == LoopType::DOUBLE_SWEEP) {
            if (dpDelta > 0) {
                loc.start = LocationInSweep::LEFT;
                loc.end = LocationInSweep::RIGHT;
                _leftSweepLoop.addOutgoingLoop(analysis);
                _rightSweepLoop.value().addIncomingLoop(analysis);
            } else {
                loc.start = LocationInSweep::RIGHT;
                loc.end = LocationInSweep::LEFT;
                _leftSweepLoop.addIncomingLoop(analysis);
                _rightSweepLoop.value().addOutgoingLoop(analysis);
            }
        } else {
            assert(behavior.loopType() == LoopType::ANCHORED_SWEEP);
            bool extendsLeft = behavior.minDpDelta() != 0;
            if ((dpDelta > 0) == extendsLeft) {
                // This is an outgoing loop
                loc.start = dpDelta > 0 ? LocationInSweep::LEFT : LocationInSweep::RIGHT;
                if (extendsLeft) {
                    _leftSweepLoop.addOutgoingLoop(analysis);
                } else {
                    _rightSweepLoop.value().addOutgoingLoop(analysis);
                }
            } else {
                // This is an incoming loop
                loc.end = dpDelta > 0 ? LocationInSweep::RIGHT : LocationInSweep::LEFT;
                if (extendsLeft) {
                    _leftSweepLoop.addIncomingLoop(analysis);
                } else {
                    _rightSweepLoop.value().addIncomingLoop(analysis);
                }
            }
        }
    }

    bool extendsLeft = _leftSweepLoop.incomingLoops().size() > 0;
    bool extendsRight = _rightSweepLoop.value().incomingLoops().size() > 0;

    if (!extendsLeft && !extendsRight) {
        // There should be at least some sweep loops
        return false;
    }

    if ((extendsLeft && _leftSweepLoop.outgoingLoops().size() == 0) ||
        (extendsRight && _rightSweepLoop.value().outgoingLoops().size() == 0)) {
        // When there are incoming loops, there must be outgoing loops
        return false;
    }

    // Check if there is a mid-sweep transition
    bool oneSideFixed = !extendsLeft || !extendsRight;
    bool hasMidTransition = false;
    for (auto &behavior : _metaLoopAnalysis->loopBehaviors()) {
        if (behavior.loopType() == LoopType::ANCHORED_SWEEP &&
            behavior.iterationDelta() != 0
        ) {
            auto &loc = _locationsInSweep[behavior.sequenceIndex()];
            if (loc.start == LocationInSweep::UNSET) {
                loc.start = oneSideFixed ? opposite(loc.end) : LocationInSweep::MID;
            } else {
                loc.end = oneSideFixed ? opposite(loc.start) : LocationInSweep::MID;
            }

            // There is a mid-sweep transition when the sweep behavior extends both ends, but there
            // is an anchored sweep loop.
            hasMidTransition |= !oneSideFixed;
        }
    }

    if (hasMidTransition) {
        _midTransition.emplace(LocationInSweep::MID);
    } else {
        // There's only a single sweep. Transfer loops registered at right to the left
        for (auto loop : _rightSweepLoop.value().incomingLoops()) {
            _leftSweepLoop.addOutgoingLoop(loop);
        }
        for (auto loop : _rightSweepLoop.value().outgoingLoops()) {
            _leftSweepLoop.addIncomingLoop(loop);
        }
        _rightSweepLoop.reset();
    }

    return true;
}

void SweepHangChecker::locateSweepTransitions() {
    LocationInSweep curLoc = LocationInSweep::UNSET;
    int loopSize = _metaLoopAnalysis->loopSize();
    for (int i = 0; i < loopSize; ++i) {
        int seqIndex = (_firstSweepLoopSeqIndex + i) % loopSize;
        auto &loc = _locationsInSweep[seqIndex];
        if (loc.start == LocationInSweep::UNSET) {
            // This is a sweep transtion
            assert(loc.end == LocationInSweep::UNSET);
            loc.start = curLoc;
            loc.end = curLoc;
        } else {
            // This is a sweep loop whose locations are already set
            assert(loc.end != LocationInSweep::UNSET);
            curLoc = loc.end;
        }
    }
}

bool SweepHangChecker::init(const MetaLoopAnalysis* metaLoopAnalysis,
                            const ExecutionState& executionState) {
    _metaLoopAnalysis = metaLoopAnalysis;

    if (!locateSweepLoops()) {
        return false;
    }
    locateSweepTransitions();

    if (!_leftTransition.analyze(*this, executionState)) {
        return false;
    }
    if (!_rightTransition.analyze(*this, executionState)) {
        return false;
    }

    if (_midTransition) {
        if (!_midTransition.value().analyze(*this, executionState)) {
            return false;
        }
    }

    if (!_leftSweepLoop.analyze(*this, executionState)) {
        return false;
    }
    if (_rightSweepLoop) {
        if (!_rightSweepLoop.value().analyze(*this, executionState)) {
            return false;
        }
    }

    // Analyze all sweeps and transitions at least once
    int cyclesToCheck = 1;

    if (!_leftTransition.isStationary()) {
        cyclesToCheck = std::max(cyclesToCheck,
                                 _leftTransition.combinedAnalysis().numBootstrapCycles());
    }
    if (!_rightTransition.isStationary()) {
        cyclesToCheck = std::max(cyclesToCheck,
                                 _rightTransition.combinedAnalysis().numBootstrapCycles());
    }
    if (_midTransition) {
        cyclesToCheck = std::max(cyclesToCheck,
                                 _midTransition.value().combinedAnalysis().numBootstrapCycles());
    }

    _proofUntil = (executionState.getRunSummary().getNumRunBlocks()
                   + _metaLoopAnalysis->loopSize() * cyclesToCheck);

    return true;
}

bool SweepHangChecker::sweepLoopContinuesForever(const ExecutionState& executionState,
                                                 SweepLoop* loop, int seqIndex) {
    auto result = loop->continuesForever(executionState);
    if (result.iteration > 0) {
        // The sweep does not continue forever as it will abort mid-sweep in a future iteration
        return false;
    }

    // Check if data value where the sweep loop exits is where it should be given the loop
    // behavior so far.
    int loopIndex = _metaLoopAnalysis->loopIndexForSequence(seqIndex);
    int prevNumIter = _metaLoopAnalysis->lastNumLoopIterations(loopIndex);
    int expectedIter = prevNumIter + loopBehavior(seqIndex).iterationDelta().value();

    auto sequenceAnalysis = _metaLoopAnalysis->sequenceAnalysis(seqIndex);
    int dpDeltaLoop = sequenceAnalysis->dataPointerDelta();
    int insIndex = result.instructionIndex % sequenceAnalysis->sequenceSize();

    auto& runSummary = executionState.getRunSummary();
    int expectedDpOffset = (expectedIter - runSummary.getLoopIteration()) * dpDeltaLoop;
    if (insIndex + 1 < sequenceAnalysis->sequenceSize()) {
        // The loop exits mid-iteration. Adjust DP accordingly.
        expectedDpOffset += sequenceAnalysis->effectiveResultAt(insIndex).dpOffset();
    }

    return expectedDpOffset == result.dpOffset;
}

bool SweepHangChecker::transitionContinuesForever(const ExecutionState& executionState,
                                                  TransitionGroup* transition, int seqIndex) {
    return transition->continuesForever(executionState);
}

// Check if a sweep loop has just started and if we should check if it runs forever.
//
// Note, as the combined results of the left- and rightward sweep loops are used, only the
// sweep loop that follows the outgoing loop for the combined analysis should be checked.
bool SweepHangChecker::verifyLoop(const ExecutionState& executionState) {
    int numRunBlocks = executionState.getRunSummary().getNumRunBlocks();
    int loopSize = _metaLoopAnalysis->loopSize();

    // The index of the loop that just started
    int seqIndex = (numRunBlocks - 1 - _metaLoopAnalysis->firstRunBlockIndex()) % loopSize;
    auto &loc = locationInSweep(seqIndex);

    if (loc.isSweepLoop()) {
        SweepLoop* loop = nullptr;
        if (loc.start == LocationInSweep::LEFT) {
            loop = &_leftSweepLoop;
        } else if (loc.start == LocationInSweep::RIGHT && _rightSweepLoop) {
            loop = &_rightSweepLoop.value();
        }
        if (loop && loop->outgoingLoopSequenceIndex() == seqIndex) {
            if (!sweepLoopContinuesForever(executionState, loop, seqIndex)) {
                return false;
            }
        }
    }

    return true;
}

// Check if a transition is about to start. If so, it should be checked that it runs
// forever.
//
// Note: It is possible that a new loop will start but there is also a non-empty
// transition that needs to be checked. This happens when there is no sequence run block that
// separates the outgoing loop from the incoming loop, but there's still an effective
// transition sequence due to incoming loop shutdown and outgoing loop bootstrap effects.
bool SweepHangChecker::verifyTransition(const ExecutionState& executionState) {
    int numRunBlocks = executionState.getRunSummary().getNumRunBlocks();
    int loopSize = _metaLoopAnalysis->loopSize();

    // The index of the run block whose execution is just about to start
    int seqIndex = (numRunBlocks - _metaLoopAnalysis->firstRunBlockIndex()) % loopSize;
    auto &loc = locationInSweep(seqIndex);

    int prevSeqIndex = (seqIndex + loopSize - 1) % loopSize;
    if (locationInSweep(prevSeqIndex).isSweepLoop()) {
        // This is the start of a transition sequence

        TransitionGroup* transition;
        switch (loc.start) {
            case LocationInSweep::LEFT: transition = &_leftTransition; break;
            case LocationInSweep::RIGHT: transition = &_rightTransition; break;
            case LocationInSweep::MID: transition = &_midTransition.value(); break;
            case LocationInSweep::UNSET: assert(false);
        }

        if (transition->incomingLoopSequenceIndex() == prevSeqIndex) {
            if (!transitionContinuesForever(executionState, transition, seqIndex)) {
                return false;
            }
        }
    }

    return true;
}

Trilian SweepHangChecker::proofHang(const ExecutionState& executionState) {
    if (executionState.getRunSummary().getNumRunBlocks() > _proofUntil) {
        return Trilian::YES;
    }

    if (executionState.getLoopRunState() == LoopRunState::STARTED) {
        if (!verifyLoop(executionState)) {
//            executionState.dumpExecutionState();
            return Trilian::NO;
        }
    } else {
        assert(executionState.getLoopRunState() == LoopRunState::ENDED);

        if (!verifyTransition(executionState)) {
//            executionState.dumpExecutionState();
            return Trilian::NO;
        }
    }

    return Trilian::MAYBE;
}

std::ostream &operator<<(std::ostream &os, const SweepHangChecker &checker) {
    os << "Left transition: " << std::endl << checker.leftTransition().combinedAnalysis();
    os << checker.leftTransition().transitionDeltas() << std::endl << std::endl;

    os << "Left sweep: " << std::endl << checker.leftSweepLoop().combinedAnalysis();
    os << checker.leftSweepLoop().sweepLoopDeltas() << std::endl << std::endl;

    if (auto &transition = checker.midTransition()) {
        os << "Mid transition: " << std::endl << transition->combinedAnalysis();
    }

    if (auto &loop = checker.rightSweepLoop()) {
        os << "Right sweep: " << std::endl << loop->combinedAnalysis();
        os << loop->sweepLoopDeltas() << std::endl << std::endl;
    }

    os << "Right transition: " << std::endl << checker.rightTransition().combinedAnalysis();
    os << checker.rightTransition().transitionDeltas() << std::endl;

    return os;
}
