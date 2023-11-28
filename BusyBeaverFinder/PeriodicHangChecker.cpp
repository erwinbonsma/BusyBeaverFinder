//
//  PeriodicHangChecker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 27/11/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "PeriodicHangChecker.h"

#include "Data.h"

void PeriodicHangChecker::init(const LoopAnalysis* loop, int loopStart) {
    _loop = loop;
    _loopStart = loopStart;

    _loopStartLastProof = -1;
}

Trilian PeriodicHangChecker::proofHangPhase1(const ExecutionState& execution) {
    const Data &data = execution.getData();

    int loopLen = (int)execution.getRunHistory().size() - _loopStart;
    if (loopLen <= _loop->loopSize() * _loop->numBootstrapCycles()) {
        // Loop is not yet fully bootstrapped. Too early to tell if the loop is hanging
        return Trilian::MAYBE;
    }

    // The detector should only be invoked at the start of a loop iteration (so that the DP-offsets
    // of the exit conditions are correct)
    assert(loopLen % _loop->loopSize() == 0);

    if (_loop->dataPointerDelta() == 0) {
        // Stationary loop

        // Check if any of the non-bootstrap conditions will be met.
        for (int i = _loop->loopSize(); --i >= 0; ) {
            const LoopExit &exit = _loop->exit(i);
            if (exit.exitWindow == ExitWindow::ANYTIME) {
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
        if (_loop->allValuesToBeConsumedAreZero(data)) {
            // This may be a hang. The only thing that can prevent this is when some values already
            // consumed by the loop cause an exit of one the "slow" non-bootstrap exits (which
            // may see values a few iterations later). Check against this by letting the loop run
            // for a few more cycles.
            _proofPhase = 2;
            _targetLoopLen = loopLen + _loop->loopSize() * _loop->numBootstrapCycles();
        } else {
            // Not all non-zero values will cause the loop to exit. That the check failed does not
            // mean the loop does not hang.
        }

        // We cannot yet conclude this is a hang.
        return Trilian::MAYBE;
    }
}

Trilian PeriodicHangChecker::proofHangPhase2(const ExecutionState& execution) {
    int loopLen = (int)execution.getRunHistory().size() - _loopStart;

    if (loopLen >= _targetLoopLen) {
        // The loop ran the required number of extra iterations without exiting. This means it
        // really hangs
        return Trilian::YES;
    } else {
        return Trilian::MAYBE;
    }
}

Trilian PeriodicHangChecker::proofHang(const ExecutionState& execution) {
    if (_loopStart != _loopStartLastProof) {
        // Reset proof state

        _proofPhase = 1;
        _loopStartLastProof = _loopStart;
    }

    return _proofPhase == 1 ? proofHangPhase1(execution) : proofHangPhase2(execution);
}
