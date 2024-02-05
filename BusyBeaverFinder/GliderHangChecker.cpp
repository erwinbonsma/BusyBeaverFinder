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
    _loopCounterIndex = ((_metaLoopAnalysis->loopRemainder(_gliderLoopIndex) + loopSize - 1)
                         % loopSize);

    // Determine the DP offset of the loop counter (compared to the start of the loop)
    _counterDpOffset = loopAnalysis->effectiveResultAt(_loopCounterIndex).dpOffset();

    if (loopAnalysis->dataPointerDelta() != 0) {
        // The glider loop should be stationary

        return false;
    }

    assert(loopBehavior.minDpDelta() == loopBehavior.maxDpDelta());
    int loopShift = loopBehavior.minDpDelta();
    auto &deltas = loopAnalysis->dataDeltas();
    int curCounterDelta = deltas.deltaAt(_counterDpOffset);
    int sumFutureDeltas = 0;
    for (auto &dd : deltas) {
        int dpDelta = dd.dpOffset() - _counterDpOffset;
        if (dpDelta == 0) continue;

        if (sign(dpDelta) != sign(loopShift)) {
            // This is data left in the wake of the glider loop
        } else if (abs(dpDelta) % abs(loopShift) != 0) {
            // Although this data value is ahead of the current loop counter, it will be skipped,
            // and also become part of the wake
        } else {
            // This modifies a future loop counter

            if (sign(curCounterDelta) == sign(dd.delta())) {
                // For now require that the next counter is always "incremented".
                return false;
            }
            sumFutureDeltas += dd.delta();
        }
    }

    if (abs(curCounterDelta) > abs(sumFutureDeltas)) {
        // Simple sanity check. Probably not needed
        return false;
    }

    return true;
}

bool GliderHangChecker::analyzeTransitionSequence(const ExecutionState& executionState) {
    auto& runHistory = executionState.getRunHistory();
    auto& runSummary = executionState.getRunSummary();

    auto &loop = _metaLoopAnalysis->loopBehaviors()[_gliderLoopIndex];
    _firstGliderLoopRunBlockIndex = _metaLoopAnalysis->firstRunBlockIndex() + loop.sequenceIndex();
    int pbIndexStart = runSummary.runBlockAt(_firstGliderLoopRunBlockIndex + 1)->getStartIndex();
    int nextLoopRunBlock = _firstGliderLoopRunBlockIndex + _metaLoopAnalysis->loopSize();

    // Include start of loop until loop exit so that DP shift is correct over an iteration of the
    // meta-loop. Or re-phrased, let the glider loop start at the instruction where it exits.
    int pbIndexEnd = runSummary.runBlockAt(nextLoopRunBlock)->getStartIndex() + _loopCounterIndex;

    if (!_transitionLoopAnalysis.analyzeLoop(&runHistory[pbIndexStart],
                                             pbIndexEnd - pbIndexStart)) {
        return false;
    }

    return true;
}

bool GliderHangChecker::init(const MetaLoopAnalysis* metaLoopAnalysis,
                             const ExecutionState& executionState) {
    _metaLoopAnalysis = nullptr;
    _proofEnd = 0;

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

    if (!analyzeTransitionSequence(executionState)) {
        return false;
    }

    // The proof checks only that all newly consumed values are zero. It may happen that
    // recently consumed values cause hang-breaking exits. Guard against this by running until
    // all loops are fully bootstrapped.
    auto loopAnalysis = _metaLoopAnalysis->loopBehaviors()[_gliderLoopIndex].loopAnalysis();
    _numBootstrapCycles = std::max(_transitionLoopAnalysis.numBootstrapCycles(),
                                   loopAnalysis->numBootstrapCycles());

    return true;
}

Trilian GliderHangChecker::proofHang(const ExecutionState& executionState) {
    // The glider loop should just have finished
    if ((executionState.getRunSummary().getNumRunBlocks() - _firstGliderLoopRunBlockIndex)
        % _metaLoopAnalysis->loopSize() != 1) {
        return Trilian::MAYBE;
    }

    if (_proofEnd == 0) {
        if (!_transitionLoopAnalysis.allValuesToBeConsumedAreZero(executionState.getData())) {
            return Trilian::MAYBE;
        }
        _proofEnd = (executionState.getRunSummary().getNumRunBlocks()
                     + _numBootstrapCycles * _metaLoopAnalysis->loopSize());
    }

    if (executionState.getRunSummary().getNumRunBlocks() >= _proofEnd) {
        return Trilian::YES;
    }

    return Trilian::MAYBE;
}
