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
    auto &runSummary = executionState.getRunSummary();
    auto &runHistory = executionState.getRunHistory();
    auto &metaAnalysis = checker._metaLoopAnalysis;
    int loopSize = metaAnalysis->loopSize();

    _analysis.analyzeMultiSequenceStart();

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

    int dpOffset = 0;
    int seqIndex = _incomingLoopSeqIndex;
    int rbIndex = metaAnalysis->firstRunBlockIndex() + seqIndex;
    for (int i = 0; i < loopSize; ++i) {
        auto &loc = checker.locationInSweep(seqIndex);
        if (loc.isSweepLoop() && loc.isAt(_location)) {
            // Add the constribution of this loop
            auto& loopBehavior = checker.loopBehavior(seqIndex);
            auto analysis = loopBehavior.loopAnalysis();
            int dpDelta = analysis->dataPointerDelta();

            // Determine number of iterations (so that all instructions that contribute to delta
            // range are included)
            //
            // dpMax(i) = dpOffset + dpMax + i * dpDelta
            // dpMin(i) = dpOffset + dpMin + i * dpDelta
            int startIter, endIter;
            if (dpDelta > 0) {
                // Iteration where loop first enters DP range: dpMax(i) >= 0
                startIter = ceildiv(-dpOffset - analysis->maxDp(), dpDelta);

                // Iteration where loop fully exited DP range: dpMin(i) >= deltaRange
                endIter = ceildiv(_deltaRange - dpOffset - analysis->minDp(), dpDelta);
            } else {
                // dpMin(i) <= deltaRange - 1  =>  i >= div(..., ...)
                startIter = ceildiv(_deltaRange - 1 - analysis->minDp() - dpOffset, dpDelta);

                // dpMax(i) < 0  =>  dpMax(i) <= -1  =>  i >= div(..., ...)
                endIter = ceildiv(-1 - dpOffset - analysis->maxDp(), dpDelta);
            }

            // Determine DP start (so that's correctly aligned with the other loops)
            int dpStart = dpOffset + startIter * dpDelta;
            auto runBlock = runSummary.runBlockAt(rbIndex);
            _analysis.analyzeMultiSequence(&runHistory.at(runBlock->getStartIndex()),
                                           (endIter - startIter) * analysis->loopSize(),
                                           dpStart);
        }

        dpOffset += metaAnalysis->dpDeltaOfRunBlock(runSummary, rbIndex);
        seqIndex = (seqIndex + 1) % metaAnalysis->loopSize();
        rbIndex += 1;
    }

    _analysis.analyzeMultiSequenceEnd(0);
}

void SweepHangChecker::SweepLoop::analyze(const SweepHangChecker& checker,
                                          const ExecutionState& executionState) {
    _incomingLoopSeqIndex = checker.findIncomingSweepLoop(_location, executionState);

    analyzeLoopAsSequence(checker, executionState);
}

void SweepHangChecker::TransitionGroup::analyzeTransitionAsLoop(const SweepHangChecker& checker,
                                                                const ExecutionState& state) {
    auto &runSummary = state.getRunSummary();
    auto &analysis = checker._metaLoopAnalysis;
    int loopSize = analysis->loopSize();

    _analysis.analyzeMultiSequenceStart();

    int dp = 0;

    // Start at instruction after incoming loop
    int seqIndex = (_incomingLoopSeqIndex + 1) % loopSize;
    int rbIndex = analysis->firstRunBlockIndex() + seqIndex;
    for (int i = 0; i < loopSize; ++i) {
         std::cout << "seqIndex = " << seqIndex << ", dp = " << dp << ", rbIndex = " << rbIndex
         << std::endl;
        auto &loc = checker.locationInSweep(seqIndex);
        if (loc.isAt(_location)) {
            if (loc.isSweepLoop()) {
                addLoopInstructions(checker, state, seqIndex, rbIndex, dp, loc.end == _location);
            } else {
                addSequenceInstructions(checker, state, rbIndex, dp);
            }
        }

        dp += analysis->dpDeltaOfRunBlock(runSummary, rbIndex);
        seqIndex = (seqIndex + 1) % loopSize;
        rbIndex += 1;
    }

    _analysis.analyzeMultiSequenceEnd(dp);
}

void SweepHangChecker::TransitionGroup::addSequenceInstructions(const SweepHangChecker& checker,
                                                                const ExecutionState& state,
                                                                int rbIndex,
                                                                int dp) {
    auto &runSummary = state.getRunSummary();
    auto runBlock = runSummary.runBlockAt(rbIndex);
    int pbIndex = runBlock->getStartIndex();

    _analysis.analyzeMultiSequence(&state.getRunHistory().at(pbIndex),
                                   runSummary.getRunBlockLength(rbIndex),
                                   dp);
}

void SweepHangChecker::TransitionGroup::addLoopInstructions(const SweepHangChecker& checker,
                                                            const ExecutionState& state,
                                                            int seqIndex, int rbIndex, int dp,
                                                            bool incoming) {
    auto &runHistory = state.getRunHistory();
    auto &runSummary = state.getRunSummary();
    auto &loopBehavior = checker.loopBehavior(seqIndex);
    auto la = loopBehavior.loopAnalysis();
    int numIter = la->numBootstrapCycles();
    int len = numIter * la->loopSize();

    // TODO: For mid-transition, also add contribution of loop that passes it

    // Range:
    // - DP-range of all sequences (which can be inside the sequence)
    // - Location LEFT/RIGHT:
    //   - DP-range of connected sweep loop (to DP = 0, i.e. first loop exit)
    // - Location MID:
    //   - For terminating loops. DP-range of connected sweep loop
    //   - For traversing loops. Do not modify DP-range but should traverse it fully

    if (incoming) {
        // Incoming loop. Execute last "numIter" full loop iterations + any remaining instructions
        auto &analysis = checker._metaLoopAnalysis;

        int dpEnd = dp + analysis->dpDeltaOfRunBlock(runSummary, rbIndex);
        int loopIndex = analysis->loopIndexForSequence(seqIndex);
        int remainder = analysis->loopRemainder(loopIndex);
        int dpDeltaLastIter = ((remainder > 0)
                               ? la->effectiveResultAt(remainder - 1).dpOffset() : 0);
        int dpStart = dpEnd - dpDeltaLastIter - numIter * la->dataPointerDelta();
        int pbIndexNext = runSummary.runBlockAt(rbIndex + 1)->getStartIndex();
        len += remainder;

        _analysis.analyzeMultiSequence(&runHistory.at(pbIndexNext - len), len, dpStart);
    } else {
        // Outgoing loop. Execute first "numIter" loop iterations
        int pbIndex = runSummary.runBlockAt(rbIndex)->getStartIndex();

        _analysis.analyzeMultiSequence(&runHistory.at(pbIndex), len, dp);
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
