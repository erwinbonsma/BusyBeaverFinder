//
//  StaticGliderHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticGliderHangDetector.h"

#include <iostream>

#include "InterpretedProgram.h"

StaticGliderHangDetector::StaticGliderHangDetector(ExhaustiveSearcher& searcher)
    : StaticHangDetector(searcher) {}

bool StaticGliderHangDetector::shouldCheckNow(bool loopContinues) {
    // Should wait for the glider-loop to finish
    return !loopContinues && _searcher.getMetaRunSummary().isInsideLoop();
}



// Assumes that the loop counter exited by the loop-counter reaching zero.
bool StaticGliderHangDetector::identifyLoopCounter() {
    RunSummary& runSummary = _searcher.getRunSummary();
    InterpretedProgram& interpretedProgram = _searcher.getInterpretedProgram();

    ProgramBlockIndex pbIndex = runSummary.getLastProgramBlockIndex();
    ProgramBlock* pb = interpretedProgram.getEntryBlock() + pbIndex;

    if (!pb->isDelta()) {
        // The last instruction should be a delta
        return false;
    }

    // Check that it changes the assumed loop counter
    int instructionIndex =
        (runSummary.getNumProgramBlocks() - _loopRunBlock->getStartIndex() - 1)
        % _loopRunBlock->getLoopPeriod();

    int startIndex = _loopRunBlock->getStartIndex();
    int i = startIndex + instructionIndex;
    int dpOffset = 0;
    while (--i >= startIndex) {
        pb = interpretedProgram.getEntryBlock() + runSummary.programBlockIndexAt(i);
        if (!pb->isDelta()) {
            dpOffset += pb->getInstructionAmount();
        }
    }

    _curCounterDpOffset = dpOffset;

    return true;
}

// Checks that loop changes two counters, the current loop counter (CC) which moves towards zero,
// and the next counter (NC) which moves away from zero by a higher amount.
bool StaticGliderHangDetector::isGliderLoop() {
    if (_loop.dataPointerDelta() != 0) {
        // The glider loop should be stationary

        return false;
    }

    if (_loop.numDataDeltas() != 2) {
        // For now, only support the most basic glider loops.

        // TODO: Extend to support more complex glider loops once programs with these behaviors are
        // encountered.
        return false;
    }

    DataDelta* ddCur = _loop.dataDeltaAt(0); // Initial assumption
    DataDelta* ddNxt;
    if (ddCur->dpOffset() != _curCounterDpOffset) {
        // Assumption was wrong
        ddNxt = ddCur;
        ddCur = _loop.dataDeltaAt(1);
    } else {
        ddNxt = _loop.dataDeltaAt(1);
    }

    _nxtCounterDpOffset = ddNxt->dpOffset();
    _curCounterDelta = ddCur->delta();
    _nxtCounterDelta = ddNxt->delta();

    if (_curCounterDelta * _nxtCounterDelta > 0) {
        // The signs should differ
        return false;
    }

    // The delta for the next loop counter should be larger (or at least equal)
    return abs(_curCounterDelta) <= abs(_nxtCounterDelta);
}

bool StaticGliderHangDetector::analyzeLoop() {
    // Assume that the loop which just finished is the glider-loop
    RunSummary& runSummary = _searcher.getRunSummary();

    _loopRunBlock = runSummary.getLastRunBlock();

    if (!_loop.analyseLoop(_searcher.getInterpretedProgram(), runSummary,
                           _loopRunBlock->getStartIndex(), _loopRunBlock->getLoopPeriod())) {
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

bool StaticGliderHangDetector::checkTransitionDeltas() {
    int totalNxtDelta = 0;
    int nxtNxtDelta = 0;
    int minDpOffset = _curCounterDpOffset; // Set it to a value that is guaranteed in range
    int maxDpOffset = _curCounterDpOffset; // Set it to a value that is guaranteed in range
    int shift = _nxtCounterDpOffset - _curCounterDpOffset;

    _transitionSequence.dump();

    for (int i = _transitionSequence.numDataDeltas(); --i >= 0; ) {
        DataDelta *dd = _transitionSequence.dataDeltaAt(i);
        int relDelta = dd->dpOffset();
        if (relDelta % shift == 0 && relDelta * shift > 0) {
            // This modifies a future loop counter. Track by how much
            totalNxtDelta += dd->delta();

            if (relDelta / shift == 2) {
                nxtNxtDelta = dd->delta();
            }
        } else {
            // Changing other values may impact how long it takes to bootstrap the hang. For
            // example, if the glider loop moves one data cell each iteration of the meta-loop and
            // the transition sequence changes a value three positions behind the current loop
            // counter, then the loop needs to have run four times before it only encounters values
            // that it changed itself.
            minDpOffset = std::min(minDpOffset, dd->dpOffset());
            maxDpOffset = std::max(maxDpOffset, dd->dpOffset());
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

    if (nxtNxtDelta == 0) {
        // For now, only detect glider hangs where the glider loop cannot handle a zero-valued next
        // loop counter

        // TODO: Extend once more complex glider hangs are encountered.
        return false;
    }

    // Calculate number of bootstrap cycles
    _numBootstrapCycles = std::max(std::abs(minDpOffset), std::abs(maxDpOffset)) / std::abs(shift);

    return (_curCounterDpOffset + _transitionSequence.dataPointerDelta()) == shift;
}

bool StaticGliderHangDetector::analyzeTransitionSequence() {
    RunSummary& runSummary = _searcher.getRunSummary();
    RunSummary& metaRunSummary = _searcher.getMetaRunSummary();
    InterpretedProgram& interpretedProgram = _searcher.getInterpretedProgram();

    RunBlock* metaRunBlock = metaRunSummary.getLastRunBlock();
    int metaPeriod = metaRunBlock->getLoopPeriod();

    // The instructions comprising the (last) transition sequence
    int startIndex =
    runSummary.runBlockAt(runSummary.getNumRunBlocks() - metaPeriod)->getStartIndex();
    int endIndex = _loopRunBlock->getStartIndex();

    if (!_transitionSequence.analyseSequence(interpretedProgram, runSummary,
                                             startIndex, endIndex - startIndex)) {
        return false;
    }

    if (!checkTransitionDeltas()) {
        return false;
    }

    return true;
}



bool StaticGliderHangDetector::analyzeHangBehaviour() {
    //std::cout <<  "Analysing" << std::endl;
    //_searcher.getInterpretedProgram().dump();
    //_searcher.dumpHangDetection();

    if (!analyzeLoop()) {
        return false;
    }

    if (!analyzeTransitionSequence()) {
        return false;
    }

    return true;
}

bool StaticGliderHangDetector::isBootstrapping() {
    RunSummary& runSummary = _searcher.getRunSummary();

    return (runSummary.getLoopIteration() < _numBootstrapCycles);
}

// Checks that the transition sequence is identical each time. I.e. if it contains loops, their
// iteration count is always fixed. This in turn means that it always does the same thing.
bool StaticGliderHangDetector::transitionSequenceIsFixed() {
    RunSummary& runSummary = _searcher.getRunSummary();
    RunSummary& metaRunSummary = _searcher.getMetaRunSummary();
    RunBlock* metaRunBlock = metaRunSummary.getLastRunBlock();
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

bool StaticGliderHangDetector::onlyZeroesAhead() {
    Data& data = _searcher.getData();
    int dpOffset = _nxtCounterDpOffset - _curCounterDpOffset;
    DataPointer dp = data.getDataPointer() + dpOffset;

    if (dpOffset > 0) {
        while (dp < data.getMaxDataP()) {
            if (*++dp) {
                return false;
            }
        }
    } else {
        while (dp > data.getMinDataP()) {
            if (*--dp) {
                return false;
            }
        }
    }

    return true;
}

Trilian StaticGliderHangDetector::proofHang() {
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
