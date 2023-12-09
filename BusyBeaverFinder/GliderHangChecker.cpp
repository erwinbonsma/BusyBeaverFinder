//
//  GliderHangChecker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 03/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "GliderHangChecker.h"

bool GliderHangChecker::identifyLoopCounters() {
    auto &loopBehavior = _metaLoopAnalysis->loopBehaviors()[_gliderLoopIndex];
    auto loopAnalysis = loopBehavior.loopAnalysis();
    int loopSize = loopAnalysis->loopSize();
    int instructionIndex = ((_metaLoopAnalysis->loopRemainder(_gliderLoopIndex) + loopSize - 1)
                            % loopSize);

    // Determine the DP offset of the loop counter (compared to the start of the loop)
    _curCounterDpOffset = loopAnalysis->effectiveResultAt(instructionIndex).dpOffset();

    if (loopAnalysis->dataPointerDelta() != 0) {
        // The glider loop should be stationary

        return false;
    }

    assert(loopBehavior.minDpDelta() == loopBehavior.maxDpDelta());
    int loopShift = loopBehavior.minDpDelta();
    auto &deltas = loopAnalysis->dataDeltas();
    int curCounterDelta = deltas.deltaAt(_curCounterDpOffset);
    bool foundNextCounter = false;
    for (auto &dd : deltas) {
        int dpDelta = dd.dpOffset() - _curCounterDpOffset;
        if (dpDelta == 0) continue;

        if (sign(dpDelta) != sign(loopShift)) {
            // This is data left in the wake of the glider loop
        } else if (abs(dpDelta) % abs(loopShift) != 0) {
            // Although this data value is ahead of the current loop counter, it will be skipped,
            // and also become part of the wake
        } else {
            // This modifies a future loop counter

            if (dpDelta == loopShift) {
                // This is the next loop counter
                foundNextCounter = true;

                if (abs(curCounterDelta) > abs(dd.delta())) {
                    // The delta for the next loop counter should be larger (or at least equal)
                    return false;
                }
            } else {
                // This bumps a counter further in the future
            }
            if (sign(curCounterDelta) == sign(dd.delta())) {
                // For now require that the next counter is always "incremented".
            }
        }
    }

    if (!foundNextCounter) {
        return false;
    }

    return true;
}

bool GliderHangChecker::init(const MetaLoopAnalysis* metaLoopAnalysis) {
    _metaLoopAnalysis = nullptr;

    _gliderLoopIndex = -1;
    int loopIndex = 0;
    for (auto &behavior : metaLoopAnalysis->loopBehaviors()) {
        if (behavior.loopType() != LoopType::GLIDER) {
            return false;
        }
        if (behavior.iterationDelta() != 0) {
            if (_gliderLoopIndex != -1) {
                // Only recognize simple glider hang programs that contain a single glider loop
                // with an increasing number of iterations. Any other loops should have a fixed
                // number of iterations (and move along with the main glider loop).
                return false;
            }

            _gliderLoopIndex = loopIndex;
        }

        loopIndex += 1;
    }

    if (_gliderLoopIndex == -1) {
        return false;
    }

    _metaLoopAnalysis = metaLoopAnalysis;

    if (!identifyLoopCounters()) {
        return false;
    }

    // TODO: Analyze transition sequence
    // Analyze sequence from glider loop exit until next glider loop start as a loop

    return true;
}

Trilian GliderHangChecker::proofHang(const ExecutionState& _execution) {
    return Trilian::MAYBE;
}
