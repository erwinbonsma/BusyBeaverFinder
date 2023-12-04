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
    int instructionIndex = _metaLoopAnalysis->loopRemainder(_gliderLoopIndex);

    // Determine the DP offset of the loop counter (compared to the start of the loop)
    _curCounterDpOffset = loopAnalysis->effectiveResultAt(instructionIndex).dpOffset();

    if (loopAnalysis->dataPointerDelta() != 0) {
        // The glider loop should be stationary

        return false;
    }

    auto &deltas = loopAnalysis->dataDeltas();
    if (deltas.size() != 2) {
        // For now, only support the most basic glider loops that increment one counter.

        return false;
    }

    bool curIndex = deltas[0].dpOffset() == _curCounterDpOffset ? 0 : 1;
    assert(deltas[curIndex].dpOffset() == _curCounterDpOffset);
    _nxtCounterDpOffset = deltas[1 - curIndex].dpOffset();

    assert(loopBehavior.minDpDelta() == loopBehavior.maxDpDelta());
    int loopShift = loopBehavior.minDpDelta();
    int dpDelta = _nxtCounterDpOffset - _curCounterDpOffset;
    if (sign(dpDelta) != sign(loopShift)) {
        // The loop should move in the direction of the next counter
        return false;
    }

    if (abs(dpDelta) % abs(loopShift) != 0) {
        // The loop should actually use the next counter as a counter
        return false;
    }

    int curCounterDelta = deltas[curIndex].delta();
    int nxtCounterDelta = deltas[1 - curIndex].delta();

    if (curCounterDelta * nxtCounterDelta > 0) {
        // The signs should differ
        return false;
    }

    if (abs(curCounterDelta) > abs(nxtCounterDelta)) {
        // The delta for the next loop counter should be larger (or at least equal)
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

    return true;
}

Trilian GliderHangChecker::proofHang(const ExecutionState& _execution) {
    return Trilian::MAYBE;
}
