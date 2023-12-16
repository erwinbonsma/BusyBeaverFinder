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
    for (auto& behavior : sweepLoops) {
        numDeltas = lcm(numDeltas, abs(behavior->loopAnalysis()->dataPointerDelta()));
    }

    int dpOffset = 0;
    for (auto& loop : sweepLoops) {
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

} // namespace v2

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
                    auto* nextBehavior = &behavior.nextLoop();
                    while (!nextBehavior->isSweepLoop()) nextBehavior = &nextBehavior->nextLoop();
                    if (sign(nextBehavior->loopAnalysis()->dataPointerDelta()) != sign(dpDelta)) {
                        _transitionGroups[1 - i].sweepLoops.push_back(&behavior);
                    }
                } else {
                    // This is an incoming loop
                    _transitionGroups[i].sweepLoops.push_back(&behavior);
                    auto* prevBehavior = &behavior.prevLoop();
                    while (!prevBehavior->isSweepLoop()) prevBehavior = &prevBehavior->prevLoop();
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

bool SweepHangChecker::initTransitionSequences(const ExecutionState& executionState) {
    return true;
}

bool SweepHangChecker::init(const MetaLoopAnalysis* metaLoopAnalysis,
                            const ExecutionState& executionState) {
    _metaLoopAnalysis = metaLoopAnalysis;

    if (!extractSweepLoops()) {
        return false;
    }

    if (!initTransitionSequences(executionState)) {
        return false;
    }

    for (auto& tg : _transitionGroups) {
        tg.initSweepLoopDeltas(metaLoopAnalysis, executionState.getRunSummary());
    }

    // TODO: Analyze stationary sequences

    // TODO: Analyze gliding sequences

    return true;
}

Trilian SweepHangChecker::proofHang(const ExecutionState& executionState) {
    // TODO: All sweep sections: value is fixed (over time) or moves away from zero

    // TODO: All stationary sequences (including mid-sweep): All values fixed or diverging.

    // TODO: All gliding sequences: Only zeroes ahead and no-recently consumed exit-values.

    return Trilian::MAYBE;
}
