//
//  SweepHangChecker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 16/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "SweepHangChecker.h"

#include "Utils.h"

void SweepHangChecker::SweepLoop::analyzeCombinedEffect(const SweepHangChecker& checker,
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

    checker.visitSweepLoopParts([this, &checker](const SweepLoopVisitState& vs) {
        auto &loc = checker.locationInSweep(vs.loopPart.seqIndex());
        if (loc.isSweepLoop() && loc.isAt(this->_location)) {
            checker.addContributionOfSweepLoopPass(vs, this->_analysis, 0, this->_deltaRange - 1);
        }
    }, executionState, _outgoingLoopSeqIndex, 0);

    _analysis.analyzeMultiSequenceEnd(0);

    // Only consider exits inside the loop range.
    _analysis.disableExits([this](LoopExit& exit) {
        int dpOffset = exit.exitCondition.dpOffset();
        return dpOffset < 0 || dpOffset >= this->_deltaRange;
    });
}

void SweepHangChecker::SweepLoop::analyze(const SweepHangChecker& checker,
                                          const ExecutionState& executionState) {
    _outgoingLoopSeqIndex = checker.findSweepLoop([this](const Location& loc) {
        return loc.start == this->_location;
    });

    analyzeCombinedEffect(checker, executionState);
}

bool SweepHangChecker::SweepLoop::continuesForever(const ExecutionState& executionState,
                                                   int dpDelta) const {
    if (_analysis.squashedDataDeltas().size() == 0) {
        // A sweep loop that does not permanently modify any values can be assumed to run forever.
        // Although loops that toggle one or more values can still terminate this never happens in
        // the context of a meta-loop, as this would happen in the first iteration of the meta-loop
        // which prevents it from looping.
        return true;
    }

    int dpDir = _location == LocationInSweep::LEFT ? 1 : -1;
    assert(dpDir * dpDelta > 0);

    int dpOffset = 0;
    while (abs(dpOffset) < abs(dpDelta)) {
        // TODO: Handle fact that analysis extends dpRange (and these values should be ignored)
        if (_analysis.stationaryLoopExits(executionState.getData(), dpOffset)) {
            return false;
        }
        dpOffset += dpDir * _deltaRange;
    }

    // TODO: Check if it is possible that the combined effects of a sweep loop shift between
    // iterations of the meta-loop. If so, handle this (by invoking check once for each offset?)

    return true;
}

void SweepHangChecker::TransitionGroup::addSequenceInstructions(const SweepLoopVisitState& vs) {
    auto &runSummary = vs.executionState.getRunSummary();
    auto &part = vs.loopPart;
    auto runBlock = runSummary.runBlockAt(part.rbIndex());
    int pbIndex = runBlock->getStartIndex();

    _analysis.analyzeMultiSequence(&vs.executionState.getRunHistory().at(pbIndex),
                                   runSummary.getRunBlockLength(part.rbIndex()),
                                   part.dpOffset());
}

void SweepHangChecker::TransitionGroup::addLoopInstructions(const SweepLoopVisitState& vs,
                                                            bool incoming) {
    auto &runHistory = vs.executionState.getRunHistory();
    auto &runSummary = vs.executionState.getRunSummary();
    auto &part = vs.loopPart;
    auto &loopBehavior = vs.checker.loopBehavior(part.seqIndex());
    auto la = loopBehavior.loopAnalysis();
    int numIter = la->numBootstrapCycles();
    int len = numIter * la->loopSize();

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

        _analysis.analyzeMultiSequence(&runHistory.at(pbIndexNext - len), len, dpStart);
    } else {
        // Outgoing loop. Execute first "numIter" loop iterations
        int pbIndex = runSummary.runBlockAt(part.rbIndex())->getStartIndex();

        _analysis.analyzeMultiSequence(&runHistory.at(pbIndex), len, part.dpOffset());
    }
}

void SweepHangChecker::TransitionGroup::analyzeLoopPartPhase1(const SweepLoopVisitState& vs) {
//    std::cout << "seqIndex = " << vs.loopPart.seqIndex()
//    << ", dp = " << vs.loopPart.dpOffset()
//    << ", rbIndex = " << vs.loopPart.rbIndex()
//    << std::endl;

    auto &loc = vs.checker.locationInSweep(vs.loopPart.seqIndex());
    if (loc.isAt(_location)) {
        if (loc.isSweepLoop()) {
            addLoopInstructions(vs, loc.end == _location);
        } else {
            addSequenceInstructions(vs);
        }
    }
}

void SweepHangChecker::TransitionGroup::analyzeLoopPartPhase2(const SweepLoopVisitState& vs) {
//    auto& part = vs.loopPart;
//    std::cout << "seqIndex = " << part.seqIndex() << ", dp = " << part.dpOffset() << std::endl;

    auto &loc = vs.checker.locationInSweep(vs.loopPart.seqIndex());
    if (loc.isSweepLoop()) {
        if (loc.start == _location) {
            vs.checker.addContributionOfSweepLoopStart(vs, _analysis, _minDp, _maxDp);
        } else if (loc.end == _location) {
            vs.checker.addContributionOfSweepLoopEnd(vs, _analysis, _minDp, _maxDp);
        } else if (_location == LocationInSweep::MID) {
            vs.checker.addContributionOfSweepLoopPass(vs, _analysis, _minDp, _maxDp);
        }
    } else {
        if (loc.isAt(_location)) {
            addSequenceInstructions(vs);
        }
    }
}

void SweepHangChecker::TransitionGroup::analyzeCombinedEffect(const SweepHangChecker& checker,
                                                              const ExecutionState& state) {
    const int loopSize = checker._metaLoopAnalysis->loopSize();

    // Start at instruction after incoming loop
    const int startSeqIndex = (_incomingLoopSeqIndex + 1) % loopSize;

    // First determine DP bounds
    _analysis.analyzeMultiSequenceStart();

    checker.visitSweepLoopParts([this](const SweepLoopVisitState& vs) {
        this->analyzeLoopPartPhase1(vs);
    }, state, startSeqIndex, 0);

    _minDp = _analysis.minDp();
    _maxDp = _analysis.maxDp();

    // Next collect all constributions within this range
    _analysis.analyzeMultiSequenceStart();
    int dpOffset = checker.visitSweepLoopParts([this](const SweepLoopVisitState& vs) {
        this->analyzeLoopPartPhase2(vs);
    }, state, startSeqIndex, 0);

    _analysis.analyzeMultiSequenceEnd(dpOffset);

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
}

void SweepHangChecker::TransitionGroup::analyze(const SweepHangChecker& checker,
                                                const ExecutionState& executionState) {
    _incomingLoopSeqIndex = checker.findSweepLoop([this](const Location& loc) {
        return loc.end == this->_location;
    });
    auto &loopBehavior = checker.loopBehavior(_incomingLoopSeqIndex);
    _isStationary = _location == LocationInSweep::MID || (_location == LocationInSweep::RIGHT
                                                          ? loopBehavior.maxDpDelta() == 0
                                                          : loopBehavior.minDpDelta() == 0);

    analyzeCombinedEffect(checker, executionState);
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

// Adds all contribitions from a sweep loop pass over the range [minDp, maxDp]
// DP offset is only used to align the traversal of the loop over the sequence. The loop always
// traverses the full range in this analysis.
void SweepHangChecker::addContributionOfSweepLoopPass(const SweepLoopVisitState vs,
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
    auto runBlock = vs.executionState.getRunSummary().runBlockAt(part.rbIndex());
    auto& runHistory = vs.executionState.getRunHistory();
    multiPartAnalysis.analyzeMultiSequence(&runHistory.at(runBlock->getStartIndex()),
                                           (endIter - startIter) * loopAnalysis->loopSize(),
                                           dpStart);
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

void SweepHangChecker::addContributionOfSweepLoopEnd(const SweepLoopVisitState vs,
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
    int pbIndexNext = runSummary.runBlockAt(part.rbIndex() + 1)->getStartIndex();
    int len = numIter * loopAnalysis->loopSize() + remainder;

    auto& runHistory = vs.executionState.getRunHistory();
    multiPartAnalysis.analyzeMultiSequence(&runHistory.at(pbIndexNext - len), len, dpStart);
}

int SweepHangChecker::visitSweepLoopParts(const SweepLoopVisitor& visitor,
                                          const ExecutionState& state,
                                          int startSeqIndex, int dpStart) const {
    auto &runSummary = state.getRunSummary();
    int loopSize = _metaLoopAnalysis->loopSize();

    struct SweepLoopPart p {
        startSeqIndex, _metaLoopAnalysis->firstRunBlockIndex() + startSeqIndex, dpStart
    };
    struct SweepLoopVisitState visitState(*this, state, p);

    for (int i = 0; i < loopSize; ++i) {
        visitor(visitState);

        p.dpOffset() += _metaLoopAnalysis->dpDeltaOfRunBlock(runSummary, p.rbIndex());
        p.seqIndex() = (p.seqIndex() + 1) % loopSize;
        p.rbIndex() += 1;
    }

    return p.dpOffset();
}

bool SweepHangChecker::locateSweepLoops() {
    _locationsInSweep.clear();
    _locationsInSweep.insert(_locationsInSweep.end(), _metaLoopAnalysis->loopSize(), {});
    _firstSweepLoopSeqIndex = _metaLoopAnalysis->loopSize();

    _midTransition.reset();
    _rightSweepLoop.reset();

    // Extract sweep loops. There should be at least two (one in each direction), and at most
    // four (when double-ended sweeps are broken up by a mid-sweep transition in both directions)
    std::array<int, 2> numOut {0, 0}, numIn {0, 0};
    for (auto &behavior : _metaLoopAnalysis->loopBehaviors()) {
        if (behavior.loopType() == LoopType::STATIONARY ||
            behavior.loopType() == LoopType::GLIDER) {
            // These loops can occur if they are small and of constant size
            if (behavior.iterationDelta() != 0) {
                return false;
            }
            continue;
        }

        // This is a sweep loop
        _firstSweepLoopSeqIndex = std::min(_firstSweepLoopSeqIndex, behavior.sequenceIndex());
        auto &loc = _locationsInSweep[behavior.sequenceIndex()];
        int dpDelta = behavior.loopAnalysis()->dataPointerDelta();

        if (behavior.loopType() == LoopType::DOUBLE_SWEEP) {
            loc.start = dpDelta > 0 ? LocationInSweep::LEFT : LocationInSweep::RIGHT;
            loc.end = dpDelta > 0 ? LocationInSweep::RIGHT : LocationInSweep::LEFT;
            numOut[loc.start == LocationInSweep::LEFT] += 1;
            numIn[loc.end == LocationInSweep::LEFT] += 1;
        } else {
            assert(behavior.loopType() == LoopType::ANCHORED_SWEEP);
            int i = behavior.minDpDelta() != 0 ? 0 : 1;
            if ((dpDelta > 0) == (i == 0)) {
                // This is an outgoing loop
                loc.start = dpDelta > 0 ? LocationInSweep::LEFT : LocationInSweep::RIGHT;
                numOut[loc.start == LocationInSweep::LEFT] += 1;
            } else {
                // This is an incoming loop
                loc.end = dpDelta > 0 ? LocationInSweep::RIGHT : LocationInSweep::LEFT;
                numIn[loc.end == LocationInSweep::LEFT] += 1;
            }
        }
    }

    if (numOut[0] != numIn[0] || numOut[1] != numIn[1]) {
        // The incoming and outgoing count should match
        return false;
    }

    if (numOut[0] == 0 && numOut[1] == 0) {
        // There should be at least some sweep loops
        return false;
    }

    bool oneSideFixed = numOut[0] == 0 || numOut[1] == 0;
    bool hasMidTransition = false;
    for (auto &behavior : _metaLoopAnalysis->loopBehaviors()) {
        if (behavior.loopType() == LoopType::ANCHORED_SWEEP) {
            auto &loc = _locationsInSweep[behavior.sequenceIndex()];
            if (loc.start == LocationInSweep::UNSET) {
                loc.start = oneSideFixed ? opposite(loc.end) : LocationInSweep::MID;
            } else {
                loc.end = oneSideFixed ? opposite(loc.start) : LocationInSweep::MID;
            }
            hasMidTransition |= !oneSideFixed;
        }
    }

    if (hasMidTransition) {
        _midTransition.emplace(LocationInSweep::MID);
        _rightSweepLoop.emplace(LocationInSweep::RIGHT);
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

    _leftTransition.analyze(*this, executionState);
    _rightTransition.analyze(*this, executionState);

    if (_midTransition) {
        _midTransition.value().analyze(*this, executionState);
    }

    _leftSweepLoop.analyze(*this, executionState);
    if (_rightSweepLoop) {
        _rightSweepLoop.value().analyze(*this, executionState);
    }

    return true;
}

bool RegularSweepHangChecker::init(const MetaLoopAnalysis* metaLoopAnalysis,
                                   const ExecutionState& executionState) {
    if (!SweepHangChecker::init(metaLoopAnalysis, executionState)) {
        return false;
    }

    int bootstrapCycles = 0;
    if (!_leftTransition.isStationary()) {
        bootstrapCycles = _leftTransition.combinedAnalysis().numBootstrapCycles();
    }
    if (!_rightTransition.isStationary()) {
        bootstrapCycles = std::max(bootstrapCycles,
                                   _rightTransition.combinedAnalysis().numBootstrapCycles());
    }
    if (_midTransition) {
        bootstrapCycles = std::max(bootstrapCycles,
                                   _midTransition.value().combinedAnalysis().numBootstrapCycles());
    }

    _proofUntil = (executionState.getRunSummary().getNumRunBlocks()
                   + _metaLoopAnalysis->loopSize() * bootstrapCycles);

    return true;
}

Trilian RegularSweepHangChecker::proofHang(const ExecutionState& executionState) {
    auto& runSummary = executionState.getRunSummary();
    int numRunBlocks = runSummary.getNumRunBlocks();

    if (numRunBlocks > _proofUntil) {
        return Trilian::YES;
    }

    int loopSize = _metaLoopAnalysis->loopSize();

    // The index of the run block whose execution is just about to start
    int seqIndex = (numRunBlocks - _metaLoopAnalysis->firstRunBlockIndex()) % loopSize;
    auto &loc = locationInSweep(seqIndex);

    if (loc.isSweepLoop()) {
        SweepLoop* loop = nullptr;
        if (loc.start == LocationInSweep::LEFT) {
            loop = &_leftSweepLoop;
        } else if (loc.start == LocationInSweep::RIGHT && _rightSweepLoop) {
            loop = &_rightSweepLoop.value();
        }
        if (loop && loop->outgoingLoopSequenceIndex() == seqIndex) {
            int loopIndex = _metaLoopAnalysis->loopIndexForSequence(seqIndex);
            int prevNumIter = _metaLoopAnalysis->lastNumLoopIterations(loopIndex);
            int expectedIter = prevNumIter + loopBehavior(seqIndex).iterationDelta().value();
            int expectedDpTravel =
                expectedIter * _metaLoopAnalysis->sequenceAnalysis(seqIndex)->dataPointerDelta();
            if (!loop->continuesForever(executionState, abs(expectedDpTravel))) {
                return Trilian::NO;
            }
        }
    }

    // Note: It is possible that a new loop now starts and there is a non-empty transition that
    // needs to be checked as well. This happens when there is no sequence run block separating the
    // outgoing loop from the incoming loop, but there's still an effective transition sequence due
    // to incoming loop shutdown and outgoing loop bootstrap effects.
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

        if (transition->incomingLoopSequenceIndex() == prevSeqIndex
            && !transition->continuesForever(executionState)) {
            return Trilian::NO;
        }
    }

    return Trilian::MAYBE;
}

// Checks that the program execution is in a meta-meta loop consisting of:
// 1: a meta-loop (where the sweep is toggling cells inside the binary-counting appendix,
// 2: a transition (which extends the appendix)
bool IrregularSweepHangChecker::checkMetaMetaLoop(const ExecutionState& executionState) {
    auto& metaMetaRunSummary = executionState.getMetaMetaRunSummary();
    auto& metaRunSummary = executionState.getMetaRunSummary();

    if (!metaMetaRunSummary.isInsideLoop()) {
        return false;
    }

    auto metaMetaRunBlock = metaMetaRunSummary.getLastRunBlock();
    constexpr int ExpectedMetaMetaLoopPeriod = 2;
    if (metaMetaRunBlock->getLoopPeriod() != ExpectedMetaMetaLoopPeriod) {
        return false;
    }

    int numMetaLoops = 0;
    for (int i = 0; i < ExpectedMetaMetaLoopPeriod; ++i) {
        int rbIndex = metaMetaRunBlock->getStartIndex() + i;
        auto metaRunBlock = metaRunSummary.runBlockAt(rbIndex);
        if (metaRunBlock->isLoop()) {
            numMetaLoops += 1;
        } else {
            if (metaRunSummary.getRunBlockLength(rbIndex) > 2) {
                // The appendix extending transition should be plain. It should contain at most
                // one loop (e.g. a different return sweep that behaves similarly to the normal
                // one)
                return false;
            }
        }
    }
    if (numMetaLoops != 1) {
        return false;
    }

    // TODO: More checks?

    return true;
}

bool IrregularSweepHangChecker::findIrregularEnds() {
    // Tracks for each sweep location:
    // 1: How many sweeps end there, 2: How many sweep ends are irregular
    std::map<LocationInSweep, std::pair<int, int>> map;

    size_t loopSize = _metaLoopAnalysis->loopSize();
    for (int i = 0; i < loopSize; ++i) {
        auto& location = locationInSweep(i);
        if (!location.isSweepLoop()) {
            continue;
        }

        int loopIndex = _metaLoopAnalysis->loopIndexForSequence(i);
        auto& behavior = _metaLoopAnalysis->loopBehaviors().at(loopIndex);

        auto& entry = map[location.end];
        entry.first += 1;

        bool endIsIrregular = !behavior.endDpGrowth();
        if (!endIsIrregular) {
            continue;
        }

        if (endIsIrregular && location.end == LocationInSweep::MID) {
            // This should not happen
            return false;
        }
        entry.second += 1;
    }

    for (auto [location, counts] : map) {
        if (!counts.second) {
            continue;
        }

        if (counts.second != counts.first) {
            // All sweeps to the same location should be irregular
            return false;
        }

        switch (location) {
            case LocationInSweep::LEFT: _leftIsIrregular = true; break;
            case LocationInSweep::RIGHT: _rightIsIrregular = true; break;
            default: assert(false);
        }
    }

    return _leftIsIrregular || _rightIsIrregular;
}

bool IrregularSweepHangChecker::init(const MetaLoopAnalysis* metaLoopAnalysis,
                                     const ExecutionState& executionState) {
    if (!checkMetaMetaLoop(executionState)) {
        return false;
    }

    if (!SweepHangChecker::init(metaLoopAnalysis, executionState)) {
        return false;
    }

    if (!findIrregularEnds()) {
        return false;
    }

    return true;
}

Trilian IrregularSweepHangChecker::proofHang(const ExecutionState& executionState) {
    return Trilian::MAYBE;
}

std::ostream &operator<<(std::ostream &os, const SweepHangChecker &checker) {
    os << "Left transition: " << std::endl << checker.leftTransition().combinedAnalysis();
    os << "Left sweep: " << std::endl << checker.leftSweepLoop().combinedAnalysis();
    if (auto &transition = checker.midTransition()) {
        os << "Mid transition: " << std::endl << transition->combinedAnalysis();
    }
    if (auto &loop = checker.rightSweepLoop()) {
        os << "Right sweep: " << std::endl << loop->combinedAnalysis();
    }
    os << "Right transition: " << std::endl << checker.rightTransition().combinedAnalysis();

    return os;
}
