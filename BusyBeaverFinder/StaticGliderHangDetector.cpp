//
//  StaticGliderHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticGliderHangDetector.h"

#include "InterpretedProgram.h"

StaticGliderHangDetector::StaticGliderHangDetector(ExhaustiveSearcher& searcher)
    : StaticHangDetector(searcher) {}

bool StaticGliderHangDetector::shouldCheckNow(bool loopContinues) {
    // Should wait for the glider-loop to finish
    return !loopContinues && _searcher.getMetaRunSummary().isInsideLoop();
}

bool StaticGliderHangDetector::analyzeHangBehaviour() {
    // Assume that the loop which just finished is the glider-loop
    RunSummary& runSummary = _searcher.getRunSummary();

    _loopRunBlock = runSummary.getLastRunBlock();

    return _loop.analyseLoop(_searcher.getInterpretedProgram(), runSummary,
                             _loopRunBlock->getStartIndex(), _loopRunBlock->getLoopPeriod());
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

    _curCounterDelta = ddCur->delta();
    _nxtCounterDelta = ddNxt->delta();

    if (_curCounterDelta * _nxtCounterDelta > 0) {
        // The signs should differ
        return false;
    }

    // The delta for the next loop counter should be larger (or at least equal)
    return abs(_curCounterDelta) <= abs(_nxtCounterDelta);
}

bool StaticGliderHangDetector::transitionChangesLoopCounter() {
    RunSummary& runSummary = _searcher.getRunSummary();
    RunSummary& metaRunSummary = _searcher.getMetaRunSummary();
    InterpretedProgram& interpretedProgram = _searcher.getInterpretedProgram();

    RunBlock* metaRunBlock = metaRunSummary.getLastRunBlock();
    int metaPeriod = metaRunBlock->getLoopPeriod();

    // The instructions comprising the (last) transition sequence
    int startIndex =
        runSummary.runBlockAt(runSummary.getNumRunBlocks() - metaPeriod)->getStartIndex();
    int endIndex = _loopRunBlock->getStartIndex();

    int nxtNxtCounterDpOffset = _nxtCounterDpOffset + (_nxtCounterDpOffset - _curCounterDpOffset);
    int nxtNxtDelta = 0;

    int dpOffset = _curCounterDpOffset;
    for (int i = startIndex; i < endIndex; i++) {
        ProgramBlock* pb = interpretedProgram.getEntryBlock() + runSummary.programBlockIndexAt(i);
        if (pb->isDelta()) {
            if (
                dpOffset != _curCounterDpOffset &&
                dpOffset != _nxtCounterDpOffset &&
                dpOffset != nxtNxtCounterDpOffset
            ) {
                // For now, let sequence only modify the loop counters, both of the current loop
                // iteration and the next. The latter is typically needed to make the next next
                // counter non-zero (so that updating it does not abort the next loop).

                // TODO: Extend once more complex transition sequences are encountered.
                return false;
            }

            if (dpOffset == nxtNxtCounterDpOffset) {
                nxtNxtDelta += pb->getInstructionAmount();
            }
        } else {
            dpOffset += pb->getInstructionAmount();
        }
    }

    if (
        abs(_curCounterDelta) == abs(_nxtCounterDelta) &&
        nxtNxtDelta * _nxtCounterDelta <= 0
    ) {
        // If in the glider loop the delta of both counter is the same, the transition sequence
        // needs to increase the (absolute value of the) next counter to ensure the hang is
        // a-periodic
        return false;
    }

    if (nxtNxtDelta == 0) {
        // For now, only detect glider hangs where the glider loop cannot handle a zero-valued next
        // loop counter

        // TODO: Extend once more complex glider hangs are encountered.
        return false;
    }

    return dpOffset == (_nxtCounterDpOffset - _curCounterDpOffset);
}

bool StaticGliderHangDetector::onlyZeroesAhead() {
    Data& data = _searcher.getData();
    int dpOffset = _nxtCounterDpOffset - _curCounterDpOffset;
    DataPointer dp = data.getDataPointer() + dpOffset;

    if (dpOffset > 0) {
        while (++dp <= data.getMaxDataP()) {
            if (*dp) {
                return false;
            }
        }
    } else {
        while (--dp >= data.getMinDataP()) {
            if (*dp) {
                return false;
            }
        }
    }

    return true;
}

Trilian StaticGliderHangDetector::proofHang() {
//    _searcher.getInterpretedProgram().dump();
//    _searcher.dumpHangDetection();

    if (!identifyLoopCounter()) {
        return Trilian::NO;
    }

    if (!isGliderLoop()) {
        return Trilian::NO;
    }

    if (!transitionChangesLoopCounter()) {
        return Trilian::NO;
    }

    if (!onlyZeroesAhead()) {
        return Trilian::NO;
    }

    return Trilian::YES;
}
