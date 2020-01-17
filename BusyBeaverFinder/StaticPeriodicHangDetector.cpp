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

    int loopLen = runSummary.getNumProgramBlocks() - loopRunBlock->getStartIndex();
    if (loopLen < loopRunBlock->getLoopPeriod() * _loop.numBootstrapCycles()) {
        // Loop is not yet stationary. Too early to tell if the loop is hanging
        return HangDetectionResult::ONGOING;
    }

    // The detector should only be invoked at the start of a loop iteration (so that the DP-offsets
    // of the exit conditions are correct)
    assert(loopLen % loopRunBlock->getLoopPeriod() == 0);

    if (_loop.dataPointerDelta() == 0) {
        for (int i = loopLen; --i >=0; ) {
            LoopExit &exit = _loop.exit(i);
            if (exit.exitWindow == ExitWindow::ANYTIME) {
                Data &data = _searcher.getData();
                int value = *(data.getDataPointer() + exit.exitCondition.dpOffset());
                if (exit.exitCondition.isTrueForValue(value)) {
                    return HangDetectionResult::FAILED;
                }
            }
        }

        // None of the exit conditions can be met
        return HangDetectionResult::HANGING;
    } else {
        for (int i = loopLen; --i >=0; ) {
            LoopExit &exit = _loop.exit(i);
            if (exit.exitWindow == ExitWindow::ANYTIME) {
                if (exit.exitCondition.isTrueForValue(0)) {
                    return HangDetectionResult::FAILED;
                }
            }
        }
    }

    return HangDetectionResult::FAILED;
}
