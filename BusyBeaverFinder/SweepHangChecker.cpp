//
//  SweepHangChecker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 16/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "SweepHangChecker.h"

bool SweepHangChecker::extractSweepLoops(const MetaLoopAnalysis* metaLoopAnalysis) {
    for (auto& tg : _transitionGroups) {
        tg.incomingLoops.clear();
        tg.outgoingLoops.clear();
    }

    // Extract sweep loops. There should be at least two (one in each direction), and at most
    // four (when double-ended sweeps are broken up by a mid-sweep transition in both directions)
    int numSweepLoops = 0;
    for (auto &behavior : metaLoopAnalysis->loopBehaviors()) {
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
                if (behavior.loopAnalysis()->dataPointerDelta() > 0) {
                    _transitionGroups[i].outgoingLoops.push_back(&behavior);
                } else {
                    _transitionGroups[i].incomingLoops.push_back(&behavior);
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

bool SweepHangChecker::init(const MetaLoopAnalysis* metaLoopAnalysis,
                            const ExecutionState& executionState) {
    if (!extractSweepLoops(metaLoopAnalysis)) {
        return false;
    }

    return true;
}

Trilian SweepHangChecker::proofHang(const ExecutionState& executionState) {
    return Trilian::MAYBE;
}
