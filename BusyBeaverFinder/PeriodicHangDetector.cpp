//
//  PeriodicHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "PeriodicHangDetector.h"

#include <iostream>

PeriodicHangDetector::PeriodicHangDetector(const ExecutionState& execution)
    : HangDetector(execution) {}

bool PeriodicHangDetector::shouldCheckNow(bool loopContinues) const {
    return loopContinues && _execution.getRunSummary().isAtEndOfLoop();
}

bool PeriodicHangDetector::analyzeHangBehaviour() {
    const RunHistory& runHistory = _execution.getRunHistory();
    const RunSummary& runSummary = _execution.getRunSummary();
    auto loopRunBlock = runSummary.getLastRunBlock();

    int loopStart = loopRunBlock->getStartIndex();
    if (!_loop.analyzeLoop(&runHistory[loopStart], loopRunBlock->getLoopPeriod())) {
        return false;
    }

    if (_loop.dataPointerDelta() != 0) {
        // A travelling loop can only hang if none of its non-bootstrap exits exit on zero.
        // As the data tape is infinite and initialized with zeros, it will always encounter zeros.
        for (int i = _loop.loopSize(); --i >= 0; ) {
            const LoopExit &exit = _loop.exit(i);
            if (exit.exitWindow == ExitWindow::ANYTIME) {
                if (exit.exitCondition.isTrueForValue(0)) {
                    return false;
                }
            }
        }
    }

    _checker.init(&_loop, loopStart);

    return true;
}

Trilian PeriodicHangDetector::proofHang() {
    return _checker.proofHang(_execution);
}
