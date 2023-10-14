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

    _loopStart = loopRunBlock->getStartIndex();
    if (!_loop.analyzeLoop(&runHistory[_loopStart], loopRunBlock->getLoopPeriod())) {
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

    return true;
}

bool PeriodicHangDetector::allValuesToBeConsumedAreBeZero() {
    const Data &data = _execution.getData();

    for (int i = _loop.loopSize(); --i >= 0; ) {
        if (_loop.exit(i).firstForValue) {
            DataPointer p = data.getDataPointer() + _loop.exit(i).exitCondition.dpOffset();
            int count = 0;

            while (
               (_loop.dataPointerDelta() > 0 && p <= data.getMaxBoundP()) ||
               (_loop.dataPointerDelta() < 0 && p >= data.getMinBoundP())
            ) {
                if (*p != 0) {
                    return false;
                }
                p += _loop.dataPointerDelta();
                count++;
                if (count > 32) {
                    // Abort. This is not expected to occur in practise for (small) programs that
                    // are actually locked in a periodic hang.
                    return false;
                }
            }
        }
    }

    return true;
}

Trilian PeriodicHangDetector::proofHangPhase1() {
    int loopLen = (int)_execution.getRunHistory().size() - _loopStart;
    if (loopLen <= _loop.loopSize() * _loop.numBootstrapCycles()) {
        // Loop is not yet fully bootstrapped. Too early to tell if the loop is hanging
        return Trilian::MAYBE;
    }

    // The detector should only be invoked at the start of a loop iteration (so that the DP-offsets
    // of the exit conditions are correct)
    assert(loopLen % _loop.loopSize() == 0);

    if (_loop.dataPointerDelta() == 0) {
        // Stationary loop

        // Check if any of the non-bootstrap conditions will be met.
        for (int i = _loop.loopSize(); --i >= 0; ) {
            const LoopExit &exit = _loop.exit(i);
            if (exit.exitWindow == ExitWindow::ANYTIME) {
                const Data &data = _execution.getData();
                int value = *(data.getDataPointer() + exit.exitCondition.dpOffset());
                if (exit.exitCondition.isTrueForValue(value)) {
                    return Trilian::NO;
                }
            }
        }

        return Trilian::YES;
    } else {
        // Travelling loop

        // A hang requires that all data values that the loop will consume are zero. Check this.
        if (allValuesToBeConsumedAreBeZero()) {
            // This may be a hang. The only thing that can prevent this is when some values already
            // consumed by the loop cause an exit of one the "slow" non-bootstrap exits (which
            // may see values a few iterations later). Check against this by letting the loop run
            // for a few more cycles.
            _proofPhase = 2;
            _targetLoopLen = loopLen + _loop.loopSize() * _loop.numBootstrapCycles();
        } else {
            // Not all non-zero values will cause the loop to exit. That the check failed does not
            // mean the loop does not hang.
        }

        // We cannot yet conclude this is a hang.
        return Trilian::MAYBE;
    }
}

Trilian PeriodicHangDetector::proofHangPhase2() {
    int loopLen = (int)_execution.getRunHistory().size() - _loopStart;

    if (loopLen >= _targetLoopLen) {
        // The loop ran the required number of extra iterations without exiting. This means it
        // really hangs
        return Trilian::YES;
    } else {
        return Trilian::MAYBE;
    }
}

Trilian PeriodicHangDetector::proofHang() {
    if (_loopStart != _loopStartLastProof) {
        // Reset proof state

        _proofPhase = 1;
        _loopStartLastProof = _loopStart;
    }

    return _proofPhase == 1 ? proofHangPhase1() : proofHangPhase2();
}

void PeriodicHangDetector::reset() {
    HangDetector::reset();

    _loopStartLastProof = -1;
}
