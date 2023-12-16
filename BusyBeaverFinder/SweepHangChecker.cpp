//
//  SweepHangChecker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 16/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "SweepHangChecker.h"

bool SweepHangChecker::extractSweepLoops() {
    for (auto& tg : _transitionGroups) {
        tg.incomingLoops.clear();
        tg.outgoingLoops.clear();
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
                int i = behavior.loopAnalysis()->dataPointerDelta() > 0 ? 1 : 0;
                _transitionGroups[i].incomingLoops.push_back(&behavior);
                _transitionGroups[1 - i].outgoingLoops.push_back(&behavior);
                numSweepLoops += 1;
                break;
            }
            case LoopType::ANCHORED_SWEEP: {
                int i = behavior.minDpDelta() != 0 ? 0 : 1;
                int dpDelta = behavior.loopAnalysis()->dataPointerDelta();
                if (dpDelta > 0) {
                    _transitionGroups[i].outgoingLoops.push_back(&behavior);
                    auto* nextBehavior = &behavior.nextLoop();
                    while (!nextBehavior->isSweepLoop()) nextBehavior = &nextBehavior->nextLoop();
                    if (nextBehavior->loopAnalysis()->dataPointerDelta() < 0) {
                        _transitionGroups[1 - i].incomingLoops.push_back(&behavior);
                    }
                } else {
                    _transitionGroups[i].incomingLoops.push_back(&behavior);
                    auto* prevBehavior = &behavior.prevLoop();
                    while (!prevBehavior->isSweepLoop()) prevBehavior = &prevBehavior->prevLoop();
                    if (prevBehavior->loopAnalysis()->dataPointerDelta() > 0) {
                        _transitionGroups[1 - i].outgoingLoops.push_back(&behavior);
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
        if (tg.incomingLoops.size() == 0 || tg.outgoingLoops.size() == 0) {
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

    // TODO: Analyze sweep section deltas

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
