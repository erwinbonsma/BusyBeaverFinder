//
//  StaticPeriodicHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticPeriodicHangDetector.h"

bool StaticPeriodicHangDetector::exhibitsHangBehaviour() {
    return _searcher.getRunSummary().isInsideLoop();
}

HangDetectionResult StaticPeriodicHangDetector::tryProofHang(bool resumed) {
    RunSummary& runSummary = _searcher.getRunSummary();
    RunBlock* loopRunBlock = runSummary.getLastRunBlock();

    if (!resumed) {
        // We are in a new loop, so classify it.
        _loop.classifyLoop(_searcher.getInterpretedProgram(), runSummary, loopRunBlock);
    }

    if (_loop.dataPointerDelta() == 0) {
        // Check: None of the exit conditions are true
    } else {
        int loopLen = runSummary.getNumProgramBlocks() - loopRunBlock->getStartIndex();

        if (loopLen < loopRunBlock->getLoopPeriod() * _loop.numBootstrapCycles()) {
            // Loop is not yet stationary. Too early to tell if the loop is hanging
            return HangDetectionResult::ONGOING;
        }

        // Check: None of the exit conditions are true
    }

    return HangDetectionResult::FAILED;
}
