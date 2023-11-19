//
//  GliderHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "GliderHangDetector.h"

#include <iostream>

#include "InterpretedProgram.h"
#include "Utils.h"

GliderHangDetector::GliderHangDetector(const ExecutionState& execution)
    : HangDetector(execution) {}

bool GliderHangDetector::shouldCheckNow(bool loopContinues) const {
    // Should wait for the glider-loop to finish
    return !loopContinues && _execution.getMetaRunSummary().isInsideLoop();
}

// Assumes that the loop counter exited by the loop-counter reaching zero.
bool GliderHangDetector::identifyLoopCounter() {
    const RunHistory& runHistory = _execution.getRunHistory();
    const ProgramBlock* pb = runHistory.back();

    if (!pb->isDelta()) {
        // The last instruction should be a delta
        return false;
    }

    // Determine the DP offset of the loop counter (compared to the start of the loop)
    int instructionIndex =
        ((int)runHistory.size() - _loopRunBlock->getStartIndex() - 1)
        % _loopRunBlock->getLoopPeriod();
    _curCounterDpOffset = _loop.effectiveResultAt(instructionIndex).dpOffset();

    return true;
}

// Checks that loop changes two counters, the current loop counter (CC) which moves towards zero,
// and the next counter (NC) which moves away from zero by a higher amount.
bool GliderHangDetector::isGliderLoop() {
    if (_loop.dataPointerDelta() != 0) {
        // The glider loop should be stationary

        return false;
    }

    if (_loop.dataDeltas().size() != 2) {
        // For now, only support the most basic glider loops.

        // TODO: Extend to support more complex glider loops once programs with these behaviors are
        // encountered.
        return false;
    }

    auto &deltas = _loop.dataDeltas();
    bool curIndex = deltas[0].dpOffset() == _curCounterDpOffset ? 0 : 1;
    _nxtCounterDpOffset = deltas[1 - curIndex].dpOffset();
    _curCounterDelta = deltas[curIndex].delta();
    _nxtCounterDelta = deltas[1 - curIndex].delta();

    if (_curCounterDelta * _nxtCounterDelta > 0) {
        // The signs should differ
        return false;
    }

    if (abs(_curCounterDelta) > abs(_nxtCounterDelta)) {
        // The delta for the next loop counter should be larger (or at least equal)
        return false;
    }

   return true;
}

bool GliderHangDetector::analyzeLoop() {
    // Assume that the loop which just finished is the glider-loop
    const RunHistory& runHistory = _execution.getRunHistory();
    const RunSummary& runSummary = _execution.getRunSummary();

    _loopRunBlock = runSummary.getLastRunBlock();

    if (!_loop.analyzeLoop(&runHistory[_loopRunBlock->getStartIndex()],
                           _loopRunBlock->getLoopPeriod())) {
        return false;
    }

    if (!identifyLoopCounter()) {
        return false;
    }

    if (!isGliderLoop()) {
        return false;
    }

    return true;
}

bool GliderHangDetector::determineCounterQueueSize() {
    int counterDistance = _nxtCounterDpOffset - _curCounterDpOffset;
    int shift = _transitionSequence.dataPointerDelta() + _curCounterDpOffset;

    if (shift == 0 || counterDistance % shift != 0) {
        // The next counter never actually becomes the current counter
        return false;
    }

    _counterQueueSize = abs(counterDistance / shift);

    return true;
}

bool GliderHangDetector::checkTransitionDeltas() {
    int totalNxtDelta = 0;
    int totalNxtNxtDelta = 0;
    int minDpOffset = _curCounterDpOffset; // Set it to a value that is guaranteed in range
    int maxDpOffset = _curCounterDpOffset; // Set it to a value that is guaranteed in range
    int counterDistance = _nxtCounterDpOffset - _curCounterDpOffset;
    int shift = _transitionSequence.dataPointerDelta() + _curCounterDpOffset;

    for (int i = maxAheadOffset; --i >= 0; ) {
        _aheadDelta[i] = 0;
    }

    for (auto &dd : _transitionSequence.dataDeltas()) {
        int relDelta = dd.dpOffset();
        bool isAhead = sign(relDelta) == sign(shift);

        if (relDelta % shift == 0 && isAhead) {
            // This modifies a future loop counter. Track by how much
            totalNxtDelta += dd.delta();

            if (relDelta / shift >= 2) {
                totalNxtNxtDelta = dd.delta();
            }
        } else {
            // Changing other values may impact how long it takes to bootstrap the hang. For
            // example, if the glider loop moves one data cell each iteration of the meta-loop and
            // the transition sequence changes a value three positions behind the current loop
            // counter, then the loop needs to have run four times before it only encounters values
            // that it changed itself.
            minDpOffset = std::min(minDpOffset, dd.dpOffset());
            maxDpOffset = std::max(maxDpOffset, dd.dpOffset());
        }

        if (sign(relDelta - counterDistance - shift) == sign(shift)) {
            // The value is ahead of the next loop counter. Update the ahead values. Loop in case
            // it is so far ahead it contributes more than once.
            //
            // Note, shifting by "counterDistance" to make the offset relative to the next loop
            // counter (instead of the current one) and shifting by "shift" to take into account
            // that at the moment of the check the next transition sequence has not yet been
            // executed.
            int dpOffset = std::abs(relDelta - counterDistance - shift);
            if (dpOffset >= maxAheadOffset) {
                return false;
            }
            assert(dpOffset < maxAheadOffset);
            while (dpOffset >= 0) {
                _aheadDelta[dpOffset] += dd.delta();
                dpOffset -= std::abs(shift);
            }
        }
    }

    if (
        abs(_curCounterDelta) == abs(_nxtCounterDelta) &&
        totalNxtDelta * _nxtCounterDelta <= 0
    ) {
        // If in the glider loop the delta of both counter is the same, the transition sequence
        // needs to increase (the absolute value of) the next counter to ensure the hang is
        // a-periodic
        return false;
    }

    if (totalNxtNxtDelta == 0) {
        // For now, only detect glider hangs where the glider loop cannot handle a zero-valued next
        // loop counter

        // TODO: Extend once more complex glider hangs are encountered.
        return false;
    }

    // Calculate number of bootstrap cycles
    _numBootstrapCycles = std::max(std::abs(minDpOffset), std::abs(maxDpOffset)) / std::abs(shift);

    return true;
}

bool GliderHangDetector::analyzeTransitionSequence() {
    const RunHistory& runHistory = _execution.getRunHistory();
    const RunSummary& runSummary = _execution.getRunSummary();
    const MetaRunSummary& metaRunSummary = _execution.getMetaRunSummary();

    const RunBlock* metaRunBlock = metaRunSummary.getLastRunBlock();
    int metaPeriod = metaRunBlock->getLoopPeriod();
    // Note: The meta-period will be two for most gliders as these consist of a Loop and a simple
    // transition sequence. However, a transition sequence can contain a fixed loop, in which it
    // consists of more that one run block. That's okay, as long as its length remains the same.

    // The instructions comprising the (last) transition sequence
    int startIndex = runSummary.runBlockAt(runSummary.getNumRunBlocks() - metaPeriod
                                           )->getStartIndex();
    int endIndex = _loopRunBlock->getStartIndex();

    _transitionSequence.analyzeSequence(&runHistory[startIndex], endIndex - startIndex);

    if (!determineCounterQueueSize()) {
        return false;
    }

    if (!checkTransitionDeltas()) {
        return false;
    }

    return true;
}

bool GliderHangDetector::analyzeHangBehaviour() {
//    std::cout <<  "Analysing" << std::endl;
//    _execution.getInterpretedProgram()->dump();
//    _execution.getRunSummary().dump();
//    _execution.getMetaRunSummary().dump();

    if (!analyzeLoop()) {
        return false;
    }

    if (!analyzeTransitionSequence()) {
        return false;
    }

    return true;
}

bool GliderHangDetector::isBootstrapping() {
    const RunSummary& runSummary = _execution.getRunSummary();

    return (runSummary.getLoopIteration() < _numBootstrapCycles);
}

// Checks that the transition sequence is identical each time. I.e. if it contains loops, their
// iteration count is always fixed. This in turn means that it always does the same thing.
bool GliderHangDetector::transitionSequenceIsFixed() {
    const RunSummary& runSummary = _execution.getRunSummary();
    const MetaRunSummary& metaRunSummary = _execution.getMetaRunSummary();
    const RunBlock* metaRunBlock = metaRunSummary.getLastRunBlock();
    int metaPeriod = metaRunBlock->getLoopPeriod();

    // Iterate over all run-blocks that comprise this meta-loop
    int startIndex = metaRunBlock->getStartIndex();
    int endIndex = runSummary.getNumRunBlocks();
    int loopRefIndex = startIndex + (runSummary.getNumRunBlocks() - startIndex - 1) % metaPeriod;
    for (int i = startIndex + metaPeriod; i < endIndex; i++) {
        int refIndex = startIndex + (i - startIndex) % metaPeriod;
        if (
            refIndex != loopRefIndex &&
            runSummary.getRunBlockLength(i) != runSummary.getRunBlockLength(refIndex)
        ) {
            return false;
        }
    }

    return true;
}

// Verify that the values ahead of the next counter match those made by the (accumulated) effect of
// the transition sequence and are zero everywhere else.
bool GliderHangDetector::onlyZeroesAhead() {
    const Data& data = _execution.getData();
    int shift = _nxtCounterDpOffset - _curCounterDpOffset;
    int delta = (shift > 0) ? 1 : -1;
    DataPointer dpStart = data.getDataPointer() + shift;
    DataPointer dpEnd = (shift > 0) ? data.getMaxDataP() : data.getMinDataP();
    DataPointer dp = dpStart;

    //_execution.dumpHangDetection();

    while (true) {
        if (dp != dpStart) {
            int dpOffset = std::abs((int)(dp - dpStart));
            if (dpOffset < maxAheadOffset) {
                if (*dp != _aheadDelta[dpOffset]) {
                    return false;
                }
            } else {
                if (*dp) { // Data value should be zero
                    return false;
                }
            }
        } else {
            // void. Ignore the next loop counter.
        }

        if (dp == dpEnd) {
            break;
        }

        dp += delta;
    }

    return true;
}

Trilian GliderHangDetector::proofHang() {
    if (isBootstrapping()) {
        return Trilian::MAYBE;
    }

    if (!transitionSequenceIsFixed()) {
        // TODO: Account for bootstrap variation?
        return Trilian::NO;
    }

    if (!onlyZeroesAhead()) {
        // TODO: Account for changes made by loop itself.
        return Trilian::MAYBE;
    }

    return Trilian::YES;
}
