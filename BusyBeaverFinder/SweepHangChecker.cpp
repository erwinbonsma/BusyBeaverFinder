//
//  SweepHangChecker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 16/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "SweepHangChecker.h"

#include "Utils.h"

namespace v2 {

void SweepTransitionGroup::initSweepLoopDeltas(const MetaLoopAnalysis* metaLoopAnalysis,
                                               const RunSummary& runSummary) {
    _sweepLoopDeltas.clear();

    int seqIndex = sweepLoops[0]->sequenceIndex();
    int rbIndex = metaLoopAnalysis->firstRunBlockIndex() + seqIndex;

    // Determine the (maximum) size of the data deltas that represent how the data changes when
    // each sweep loop traversed the data once. It is the LCM of the (absolute) DP deltas of each
    // loop.
    //
    // E.g. if one sweep loop moves DP two spots each iteration with data deltas [1, 2] and the
    // other sweep loop moves DP three spots with data deltas [0, 3, 6] the resulting deltas are
    // [1, 5, 7, 2, 4, 8]
    int numDeltas = 1;
    for (auto loop : sweepLoops) {
        numDeltas = lcm(numDeltas, abs(loop->loopAnalysis()->dataPointerDelta()));
    }

    int dpOffset = 0;
    for (auto loop : sweepLoops) {
        // Correctly align the data deltas of this loop
        int nxtSeqIndex = loop->sequenceIndex();
        while (seqIndex != nxtSeqIndex) {
            dpOffset += metaLoopAnalysis->dpDeltaOfRunBlock(runSummary, rbIndex);
            seqIndex = (seqIndex + 1) % metaLoopAnalysis->loopSize();
            rbIndex += 1;
        }

        // Add the data deltas of this loop
        for (auto& dd : loop->loopAnalysis()->squashedDataDeltas()) {
            int deltaRange = abs(loop->loopAnalysis()->dataPointerDelta());
            for (int i = 0; i < numDeltas; i += deltaRange) {
                int dpDelta = normalizedMod(dpOffset + dd.dpOffset() + i, numDeltas);
                _sweepLoopDeltas.addDelta(dpDelta, dd.delta());
            }
        }
    }
}

void SweepTransitionGroup::analyzeTransition(const MetaLoopAnalysis* mla,
                                             const ExecutionState& state) {
    auto &runHistory = state.getRunHistory();
    auto &runSummary = state.getRunSummary();

    _transitionDeltas.clear();

    // First collect the contribution of the sequences (thereby also determining the DP range)
    int dp = 0;
    bool isAtSweepEnd = true; // The first loop is an incoming loop
    auto lastSweepLoop = sweepLoops[0];

    // Start at instruction after incoming loop
    int seqIndex = (lastSweepLoop->sequenceIndex() + 1) % mla->loopSize();

    int rbIndex = mla->firstRunBlockIndex() + seqIndex;
    auto nextSweepLoop = lastSweepLoop->nextLoop();
    for (int i = mla->loopSize(); --i >= 0; ) {
        if (isAtSweepEnd) {
            const DataDeltas* dd = nullptr;
            if (mla->isLoop(seqIndex)) {
                if (seqIndex != nextSweepLoop->sequenceIndex()) {
                    // This is a fixed-size loop
                    assert(mla->loopIterationDelta(mla->loopIndexForSequence(seqIndex)) == 0);
                    auto sa = mla->unrolledLoopSequenceAnalysis(state, seqIndex);
                    dd = &sa->dataDeltas();
                }
            } else {
                auto sa = mla->sequenceAnalysisResults()[seqIndex];
                dd = &sa->dataDeltas();
            }
            if (dd) {
                for (auto delta : *dd) {
                    _transitionDeltas.addDelta(delta.dpOffset(), delta.delta());
                }
            }
        }

        dp += mla->dpDeltaOfRunBlock(runSummary, rbIndex);
        if (seqIndex == nextSweepLoop->sequenceIndex()) {
            isAtSweepEnd = !isAtSweepEnd;
            lastSweepLoop = nextSweepLoop;
            nextSweepLoop = lastSweepLoop->nextLoop();
        }
        rbIndex += 1;
        seqIndex = (seqIndex + 1) % mla->loopSize();
    }

    // Next, now the range is known, add the contributions from the sweep loops
    dp = 0;
    lastSweepLoop = sweepLoops[0];
    seqIndex = lastSweepLoop->sequenceIndex() + 1; // Start at instruction after incoming loop
    rbIndex = mla->firstRunBlockIndex() + seqIndex;
    nextSweepLoop = lastSweepLoop->nextLoop();
    bool outgoing = true;
    int minDp = _transitionDeltas.minDpOffset();
    int maxDp = _transitionDeltas.maxDpOffset();
    auto isDeltaInRange = [=](int dp) { return (dp >= minDp && dp <= maxDp); };
    for (int i = mla->loopSize(); --i >= 0; ) {
        if (seqIndex == nextSweepLoop->sequenceIndex()) {
            int loopIndex = mla->loopIndexForSequence(seqIndex);
            auto &behavior = mla->loopBehaviors()[loopIndex];
            auto la = behavior.loopAnalysis();
            int pbIndex = runSummary.runBlockAt(rbIndex)->getStartIndex();

            if (outgoing) {
                // Outgoing loop: add deltas until its out of range

                // Calculate how many iterations before all modifications of the loop are beyond
                // the stationary data delta range.
                int numIter = (atRight
                               ? (minDp - dp - la->dataDeltas().maxDpOffset()
                                  ) / la->dataPointerDelta()
                               : (maxDp - dp - la->dataDeltas().minDpOffset()
                                  ) / la->dataPointerDelta()) + 1;
                assert(numIter >= 1);

                auto begin = runHistory.cbegin() + pbIndex;
                auto end = begin + la->loopSize() * numIter;
                _transitionDeltas.bulkAdd(begin, end, dp, isDeltaInRange);
            } else {
                // Incoming loop: Determine which iteration the loop comes in range, and run it
                // until it terminates
                //
                // Note: As the loop may not fully execute its last iteration, deltas from its
                // last iteration could theoretically fall outside the loop's data delta range.
                // Ignoring this for now.

                int dpEnd = dp + mla->dpDeltaOfRunBlock(runSummary, rbIndex);
                int remainder = mla->loopRemainder(loopIndex);
                int dpDeltaLastIter = ((remainder > 0)
                                       ? la->effectiveResultAt(remainder).dpOffset() : 0);
                int numIter = (atRight
                               ? (dpEnd + la->dataDeltas().maxDpOffset() - minDp - dpDeltaLastIter
                                  ) / la->dataPointerDelta()
                               : (dpEnd + la->dataDeltas().minDpOffset() - maxDp - dpDeltaLastIter
                                  ) / la->dataPointerDelta()) + 1;
                assert(numIter >= 1);
                int dpStart = dpEnd - dpDeltaLastIter - numIter * la->dataPointerDelta();
                int pbIndexNext = runSummary.runBlockAt(rbIndex + 1)->getStartIndex();

                auto end = runHistory.cbegin() + pbIndexNext;
                auto begin = end - remainder - la->loopSize() * numIter;
                _transitionDeltas.bulkAdd(begin, end, dpStart, isDeltaInRange);
            }
        }

        dp += mla->dpDeltaOfRunBlock(runSummary, rbIndex);
        if (seqIndex == nextSweepLoop->sequenceIndex()) {
            outgoing = !outgoing;
            lastSweepLoop = nextSweepLoop;
            nextSweepLoop = lastSweepLoop->nextLoop();
        }
        rbIndex += 1;
        seqIndex = (seqIndex + 1) % mla->loopSize();
    }
}


void SweepTransitionGroup::analyze(const MetaLoopAnalysis* metaLoopAnalysis,
                                   const ExecutionState& executionState) {
    int i = 0;
    for (auto loop : sweepLoops) {
        bool stationary = atRight ? loop->maxDpDelta() == 0 : loop->minDpDelta() == 0;
        if (i == 0) {
            _isStationary = stationary;
        } else {
            assert(_isStationary == stationary);
        }
    }

    // Ensure that the first loop is an incoming sweep
    auto firstLoop = sweepLoops[0];
    bool firstIsIncoming = atRight == (firstLoop->loopAnalysis()->dataPointerDelta() > 0);
    if (!firstIsIncoming) {
        sweepLoops.erase(sweepLoops.begin());
        sweepLoops.push_back(firstLoop);
    }

    initSweepLoopDeltas(metaLoopAnalysis, executionState.getRunSummary());
    analyzeTransition(metaLoopAnalysis, executionState);
}

} // namespace v2

SweepHangChecker::SweepHangChecker() {
    bool atRight = false;
    for (auto& tg : _transitionGroups) {
        tg.atRight = atRight;
        atRight = !atRight;
    }
}

bool SweepHangChecker::extractSweepLoops() {
    for (auto& tg : _transitionGroups) {
        tg.sweepLoops.clear();
    }

    // Extract sweep loops. There should be at least two (one in each direction), and at most
    // four (when double-ended sweeps are broken up by a mid-sweep transition in both directions)
    int numSweepLoops = 0;
    for (auto &behavior : _metaLoopAnalysis->loopBehaviors()) {
        switch (behavior.loopType()) {
            case LoopType::STATIONARY:
            case LoopType::GLIDER: {
                // This may be a small, fixed-size loop at the end of a sweep
                if (behavior.iterationDelta() != 0) {
                    return false;
                }
                break;
            }
            case LoopType::DOUBLE_SWEEP: {
                _transitionGroups[0].sweepLoops.push_back(&behavior);
                _transitionGroups[1].sweepLoops.push_back(&behavior);
                numSweepLoops += 1;
                break;
            }
            case LoopType::ANCHORED_SWEEP: {
                int i = behavior.minDpDelta() != 0 ? 0 : 1;
                int dpDelta = behavior.loopAnalysis()->dataPointerDelta();
                if ((dpDelta > 0) == (i == 0)) {
                    // This is an outgoing loop
                    _transitionGroups[i].sweepLoops.push_back(&behavior);
                    auto* nextBehavior = behavior.nextLoop();
                    while (!nextBehavior->isSweepLoop()) nextBehavior = nextBehavior->nextLoop();
                    if (sign(nextBehavior->loopAnalysis()->dataPointerDelta()) != sign(dpDelta)) {
                        _transitionGroups[1 - i].sweepLoops.push_back(&behavior);
                    }
                } else {
                    // This is an incoming loop
                    _transitionGroups[i].sweepLoops.push_back(&behavior);
                    auto* prevBehavior = behavior.prevLoop();
                    while (!prevBehavior->isSweepLoop()) prevBehavior = prevBehavior->prevLoop();
                    if (sign(prevBehavior->loopAnalysis()->dataPointerDelta()) != sign(dpDelta)) {
                        _transitionGroups[1 - i].sweepLoops.push_back(&behavior);
                    }
                }
                numSweepLoops += 1;
            }
        }
    }

    if (numSweepLoops != 2) {
        // TODO: Extend to support mid-sweep transitions
        // TODO: Extend to support meta-loops consisting of more than one meta-run loop iteration
        return false;
    }

    for (auto& tg : _transitionGroups) {
        if (tg.sweepLoops.size() == 0 || tg.sweepLoops.size() % 2 != 0) {
            return false;
        }
    }

    return true;
}

bool SweepHangChecker::init(const MetaLoopAnalysis* metaLoopAnalysis,
                            const ExecutionState& executionState) {
    _metaLoopAnalysis = metaLoopAnalysis;

    if (!extractSweepLoops()) {
        return false;
    }

    for (auto& tg : _transitionGroups) {
        tg.analyze(metaLoopAnalysis, executionState);
    }

    // TODO: Analyze mid-sweep transition (if any)

    return true;
}

Trilian SweepHangChecker::proofHang(const ExecutionState& executionState) {
    // TODO: All sweep sections: value is fixed (over time) or moves away from zero

    // TODO: All stationary sequences (including mid-sweep): All values fixed or diverging.

    // TODO: All gliding sequences: Only zeroes ahead and no-recently consumed exit-values.

    return Trilian::MAYBE;
}
