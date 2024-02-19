//
//  SweepHangChecker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 16/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "SweepHangChecker.h"

#include "Utils.h"

void SweepHangChecker::SweepLoop::analyzeLoopAsSequence(const SweepHangChecker& checker,
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
    }, executionState, _incomingLoopSeqIndex, 0);

    _analysis.analyzeMultiSequenceEnd(0);
}

void SweepHangChecker::SweepLoop::analyze(const SweepHangChecker& checker,
                                          const ExecutionState& executionState) {
    _incomingLoopSeqIndex = checker.findIncomingSweepLoop(_location, executionState);

    analyzeLoopAsSequence(checker, executionState);
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

void SweepHangChecker::TransitionGroup::analyzeTransitionAsLoop(const SweepHangChecker& checker,
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
}

void SweepHangChecker::TransitionGroup::analyze(const SweepHangChecker& checker,
                                                const ExecutionState& executionState) {
    _incomingLoopSeqIndex = checker.findIncomingSweepLoop(_location, executionState);
    auto &loopBehavior = checker.loopBehavior(_incomingLoopSeqIndex);
    _isStationary = _location == LocationInSweep::MID || (_location == LocationInSweep::RIGHT
                                                          ? loopBehavior.maxDpDelta() == 0
                                                          : loopBehavior.minDpDelta() == 0);

    analyzeTransitionAsLoop(checker, executionState);
}

int SweepHangChecker::findIncomingSweepLoop(LocationInSweep location,
                                            const ExecutionState& executionState) const {
    int loopSize = _metaLoopAnalysis->loopSize();

    for (int i = 0; i < loopSize; ++i) {
        auto& loc = locationInSweep(i);
        if (loc.isAt(location) && loc.isSweepLoop() && loc.end == location) {
            // Found an incoming sweep loop
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

Trilian SweepHangChecker::proofHang(const ExecutionState& executionState) {
    // TODO: All sweep sections: value is fixed (over time) or moves away from zero

    // TODO: All stationary sequences (including mid-sweep): All values fixed or diverging.

    // TODO: All gliding sequences: Only zeroes ahead and no-recently consumed exit-values.

    return Trilian::MAYBE;
}
