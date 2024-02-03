//
//  SweepHangChecker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 16/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "SweepHangChecker.h"

#include "Utils.h"

void SweepHangChecker::SweepLoop::initSweepLoopDeltas(const SweepHangChecker& checker,
                                                      const RunSummary& runSummary) {
    _sweepLoopDeltas.clear();

    auto &analysis = checker._metaLoopAnalysis;
    int loopSize = analysis->loopSize();

    // Determine the (maximum) size of the data deltas that represent how the data changes when
    // each sweep loop traversed the data once. It is the LCM of the (absolute) DP deltas of each
    // loop.
    //
    // E.g. if one sweep loop moves DP two spots each iteration with data deltas [1, 2] and the
    // other sweep loop moves DP three spots with data deltas [0, 3, 6] the resulting deltas are
    // [1, 5, 7, 2, 4, 8]
    int numDeltas = 1;
    for (int i = 0; i < loopSize; ++i) {
        auto &loc = checker.locationInSweep(i);
        if (loc.isSweepLoop() && loc.isAt(_location)) {
            auto &loopBehavior = checker.loopBehavior(i);
            numDeltas = lcm(numDeltas, abs(loopBehavior.loopAnalysis()->dataPointerDelta()));
        }
    }

    int dpOffset = 0;
    int seqIndex = _incomingLoopSeqIndex;
    int rbIndex = analysis->firstRunBlockIndex() + seqIndex;
    for (int i = 0; i < loopSize; ++i) {
        auto &loc = checker.locationInSweep(seqIndex);
        if (loc.isSweepLoop() && loc.isAt(_location)) {
            // Add the data deltas of this loop
            auto& loopBehavior = checker.loopBehavior(seqIndex);
            int deltaRange = abs(loopBehavior.loopAnalysis()->dataPointerDelta());
            for (auto& dd : loopBehavior.loopAnalysis()->squashedDataDeltas()) {
                for (int j = 0; j < numDeltas; j += deltaRange) {
                    int dpDelta = normalizedMod(dpOffset + dd.dpOffset() + j, numDeltas);
                    _sweepLoopDeltas.updateDelta(dpDelta, dd.delta());
                }
            }
        }

        dpOffset += analysis->dpDeltaOfRunBlock(runSummary, rbIndex);
        seqIndex = (seqIndex + 1) % analysis->loopSize();
        rbIndex += 1;
    }
}

void SweepHangChecker::SweepLoop::analyze(const SweepHangChecker& checker,
                                          const ExecutionState& executionState) {
    auto &analysis = checker._metaLoopAnalysis;
    int loopSize = analysis->loopSize();
    _incomingLoopSeqIndex = -1;

    for (int i = 0; i < loopSize; ++i) {
        auto& loc = checker.locationInSweep(i);
        if (loc.isAt(_location) && loc.isSweepLoop()) {
            if (loc.end == _location) {
                // Found an incoming sweep loop
                _incomingLoopSeqIndex = i;
                break;
            }
        }
    }

    initSweepLoopDeltas(checker, executionState.getRunSummary());
}

void SweepHangChecker::TransitionGroup::addDeltasFromTransitions(const SweepHangChecker& checker,
                                                                 const ExecutionState& state) {
    auto &runHistory = state.getRunHistory();
    auto &runSummary = state.getRunSummary();
    auto &analysis = checker._metaLoopAnalysis;
    int loopSize = analysis->loopSize();

    _transitionDeltas.clear();

    int dp = 0;

    // Start at instruction after incoming loop
    int seqIndex = (_incomingLoopSeqIndex + 1) % loopSize;
    int rbIndex = analysis->firstRunBlockIndex() + seqIndex;
    for (int i = 0; i < loopSize; ++i) {
//        std::cout << "seqIndex = " << seqIndex << ", dp = " << dp << ", rbIndex = " << rbIndex
//        << std::endl;
        auto &loc = checker.locationInSweep(seqIndex);
        if (loc.isAt(_location)) {
            if (!loc.isSweepLoop()) {
                const DataDeltas* dd = nullptr;
                if (analysis->isLoop(seqIndex)) {
                    // This is a fixed-size loop
                    assert(analysis->loopIterationDelta(analysis->loopIndexForSequence(seqIndex)
                                                        ) == 0);
                    auto sa = analysis->unrolledLoopSequenceAnalysis(state, seqIndex);
                    dd = &sa->dataDeltas();
                } else {
                    auto sa = analysis->sequenceAnalysis(seqIndex);
                    dd = &sa->dataDeltas();
                }
                for (auto delta : *dd) {
                    _transitionDeltas.updateDelta(delta.dpOffset() + dp, delta.delta());
                }
//                std::cout << "seq dd: " << *dd << std::endl;
//                std::cout << "deltas: " << _transitionDeltas << std::endl;
            } else if (loc.end == _location) {
                // Check if the incoming loop has a remainder. If so, also add this. This needs
                // to be done here, as sometimes the only delta realized at a transition is from
                // a pre-mature exit, which may be outside of the transition range otherwise.
                auto &loopBehavior = checker.loopBehavior(seqIndex);
                auto la = loopBehavior.loopAnalysis();
                int loopIndex = analysis->loopIndexForSequence(seqIndex);
                int remainder = analysis->loopRemainder(loopIndex);
                if (remainder > 0) {
                    int dpEnd = dp + analysis->dpDeltaOfRunBlock(runSummary, rbIndex);
                    int dpDeltaLastIter = la->effectiveResultAt(remainder - 1).dpOffset();
                    int dpStart = dpEnd - dpDeltaLastIter;
                    int pbIndexNext = runSummary.runBlockAt(rbIndex + 1)->getStartIndex();

                    auto end = runHistory.cbegin() + pbIndexNext;
                    auto begin = end - remainder;
                    _transitionDeltas.bulkAdd(begin, end, dpStart);

//                    std::cout << "deltas: " << _transitionDeltas << std::endl;
                }
            }
        }

        dp += analysis->dpDeltaOfRunBlock(runSummary, rbIndex);
        seqIndex = (seqIndex + 1) % loopSize;
        rbIndex += 1;
    }
}

void SweepHangChecker::TransitionGroup::addDeltasFromSweepLoops(const SweepHangChecker& checker,
                                                                const ExecutionState& state) {
    auto &runHistory = state.getRunHistory();
    auto &runSummary = state.getRunSummary();
    auto &analysis = checker._metaLoopAnalysis;
    int loopSize = analysis->loopSize();

//    std::cout << "atRight = " << (_location == LocationInSweep::RIGHT)
//    << ", deltas: " << _transitionDeltas << std::endl;

    int dp = 0;
    // Start at instruction after incoming loop
    int seqIndex = (_incomingLoopSeqIndex + 1) % loopSize;
    int rbIndex = analysis->firstRunBlockIndex() + seqIndex;
    int minDp = _transitionDeltas.minDpOffset();
    int maxDp = _transitionDeltas.maxDpOffset();
    auto isDeltaInRange = [=](int dp) { return (dp >= minDp && dp <= maxDp); };
    for (int i = 0; i < loopSize; ++i) {
//        std::cout << "seqIndex = " << seqIndex << ", dp = " << dp << ", rbIndex = " << rbIndex
//        << std::endl;

        auto &loc = checker.locationInSweep(seqIndex);
        if (loc.isAt(_location) && loc.isSweepLoop()) {
            auto &loopBehavior = checker.loopBehavior(seqIndex);
            auto la = loopBehavior.loopAnalysis();
            int pbIndex = runSummary.runBlockAt(rbIndex)->getStartIndex();

            if (loc.start == _location) {
                // Outgoing loop: add deltas until its out of range

                // Calculate how many iterations before all modifications of the loop are beyond
                // the stationary data delta range.
                int numIter = (_location == LocationInSweep::RIGHT
                               ? (minDp - dp - la->dataDeltas().maxDpOffset()
                                  ) / la->dataPointerDelta()
                               : (maxDp - dp - la->dataDeltas().minDpOffset()
                                  ) / la->dataPointerDelta()) + 1;
                numIter = std::max(numIter, 0);

                auto begin = runHistory.cbegin() + pbIndex;
                auto end = begin + la->loopSize() * numIter;
                _transitionDeltas.bulkAdd(begin, end, dp, isDeltaInRange);
//                std::cout << "dd: " << _transitionDeltas << std::endl;
            } else {
                // Incoming loop: Determine which iteration the loop comes in range, and run it
                // until it terminates
                //
                // Note: As the loop may not fully execute its last iteration, deltas from its
                // last iteration could theoretically fall outside the loop's data delta range.
                // Ignoring this for now.

                int dpEnd = dp + analysis->dpDeltaOfRunBlock(runSummary, rbIndex);
                int loopIndex = analysis->loopIndexForSequence(seqIndex);
                int remainder = analysis->loopRemainder(loopIndex);
                int dpDeltaLastIter = ((remainder > 0)
                                       ? la->effectiveResultAt(remainder - 1).dpOffset() : 0);
                int numIter = (_location == LocationInSweep::RIGHT
                               ? (dpEnd + la->dataDeltas().maxDpOffset() - minDp - dpDeltaLastIter
                                  ) / la->dataPointerDelta()
                               : (dpEnd + la->dataDeltas().minDpOffset() - maxDp - dpDeltaLastIter
                                  ) / la->dataPointerDelta()) + 1;
                // The number of complete iterations that the loop traverses the sequence can be
                // zero when the DP delta in its last iteration is large (and outside its
                // effective squashed DP range). The calculation can even (incorrectly) get a
                // negative value. Fix this (and err on the side of caution).
                numIter = std::max(numIter, 2);
                int dpStart = dpEnd - dpDeltaLastIter - numIter * la->dataPointerDelta();
                int pbIndexNext = runSummary.runBlockAt(rbIndex + 1)->getStartIndex();

                auto end = runHistory.cbegin() + pbIndexNext - remainder;
                auto begin = end - la->loopSize() * numIter;
                _transitionDeltas.bulkAdd(begin, end, dpStart, isDeltaInRange);
//                std::cout << "dd: " << _transitionDeltas << std::endl;
            }
        }

        dp += analysis->dpDeltaOfRunBlock(runSummary, rbIndex);
        seqIndex = (seqIndex + 1) % loopSize;
        rbIndex += 1;
    }
}

void SweepHangChecker::TransitionGroup::analyze(const SweepHangChecker& checker,
                                                const ExecutionState& executionState) {
    auto &analysis = checker._metaLoopAnalysis;
    int loopSize = analysis->loopSize();
    _incomingLoopSeqIndex = -1;

    for (int i = 0; i < loopSize; ++i) {
        auto& loc = checker.locationInSweep(i);
        if (loc.isAt(_location) && loc.isSweepLoop()) {
            if (loc.end == _location) {
                // Found an incoming sweep loop
                auto &loopBehavior = checker.loopBehavior(i);
                _incomingLoopSeqIndex = i;
                _isStationary = (_location == LocationInSweep::RIGHT
                                 ? loopBehavior.maxDpDelta() == 0
                                 : loopBehavior.minDpDelta() == 0);
                break;
            }
        }
    }

    // First collect the contribution of the sequences (thereby also determining the DP range)
    addDeltasFromTransitions(checker, executionState);

    // Next, now the range is known, add the contributions from the sweep loops
    addDeltasFromSweepLoops(checker, executionState);
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
