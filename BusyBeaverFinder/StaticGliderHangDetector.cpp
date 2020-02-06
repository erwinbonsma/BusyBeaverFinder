//
//  StaticGliderHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticGliderHangDetector.h"

#include "InterpretedProgram.h"


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

// Checks that loop changes two counters, the current loop counter (CC) which moves towards zero,
// and the next counter (NC) which moves away from zero by a higher amount.
bool StaticGliderHangDetector::isGliderLoop(int &curCounterDpOffset, int &nxtCounterDpOffset) {
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

    int absDelta0 = abs(_loop.dataDeltaAt(0)->delta());
    int absDelta1 = abs(_loop.dataDeltaAt(1)->delta());

    if (absDelta0 == absDelta1) {
        // The amount of change should differ for the hang to be a-periodic
        return false;
    }

    curCounterDpOffset = _loop.dataDeltaAt(absDelta0 > absDelta1)->dpOffset();
    nxtCounterDpOffset = _loop.dataDeltaAt(absDelta0 < absDelta1)->dpOffset();

    return true;
}

// Check that the last executed instruction, which caused an exit of the glider loop, is changing
// the loop counter.
bool StaticGliderHangDetector::exitingAtLoopCounterChange(int currentCounterDpOffset) {
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
        (runSummary.getNumRunBlocks() - _loopRunBlock->getStartIndex() - 1)
        % _loopRunBlock->getLoopPeriod();

    int i = instructionIndex;
    int dpOffset = 0;
    pb = interpretedProgram.getEntryBlock();
    while (--i >= 0) {
        if (!pb->isDelta()) {
            dpOffset += pb->getInstructionAmount();
        }
    }

    return (dpOffset == currentCounterDpOffset);
}

bool StaticGliderHangDetector::transitionChangesLoopCounter(int targetDpShift) {
    RunSummary& runSummary = _searcher.getRunSummary();
    RunSummary& metaRunSummary = _searcher.getMetaRunSummary();
    InterpretedProgram& interpretedProgram = _searcher.getInterpretedProgram();

    RunBlock* metaRunBlock = metaRunSummary.getLastRunBlock();
    int metaPeriod = metaRunBlock->getLoopPeriod();

    // The instructions comprising the (last) transition sequence
    int startIndex =
        runSummary.runBlockAt(runSummary.getNumRunBlocks() - metaPeriod)->getStartIndex();
    int endIndex = _loopRunBlock->getStartIndex();

    int dpOffset = 0;
    for (int i = startIndex; i < endIndex; i++) {
        ProgramBlock* pb = interpretedProgram.getEntryBlock() + runSummary.programBlockIndexAt(i);
        if (pb->isDelta()) {
            if (dpOffset != 0 && dpOffset != targetDpShift) {
                // For now, let sequence only modify both counters.

                // TODO: Extend once more complex transition sequences are encountered.
                return false;
            }
        } else {
            dpOffset += pb->getInstructionAmount();
        }
    }

    return (dpOffset == targetDpShift);

    return true;
}

bool StaticGliderHangDetector::onlyZeroesAhead(int dpOffset) {
    Data& data = _searcher.getData();
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
    int curCounterDpOffset, nxtCounterDpOffset;

    if (!isGliderLoop(curCounterDpOffset, nxtCounterDpOffset)) {
        return Trilian::NO;
    }

    if (!exitingAtLoopCounterChange(curCounterDpOffset)) {
        return Trilian::NO;
    }

    if (!transitionChangesLoopCounter(nxtCounterDpOffset - curCounterDpOffset)) {
        return Trilian::NO;
    }

    if (!onlyZeroesAhead(nxtCounterDpOffset - curCounterDpOffset)) {
        return Trilian::NO;
    }

    return Trilian::NO; // TODO
}
