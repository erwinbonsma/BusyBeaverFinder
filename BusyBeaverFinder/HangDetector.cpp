//
//  HangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "HangDetector.h"

#include <cassert>
#include <iostream>

HangDetector::HangDetector(const ExecutionState& execution) :
    _execution(execution)
{
    reset();
}

void HangDetector::reset() {
    _lastFailedCheckPoint = 0;
    _analysisCheckPoint = 0;

    clearAnalysis();
}

bool HangDetector::detectHang() {
    if (!shouldCheckNow()) {
        return false;
    }

    size_t now = currentCheckPoint(); // + (_execution.getLoopRunState() == LoopRunState::ENDED);

    if (now == _lastFailedCheckPoint) {
        // We already checked this and it failed. Ignore.
        return false;
    }

    if (_analysisCheckPoint != now) {
        // We have not yet analysed the current situation
        if (!analyzeHangBehaviour()) {
            _lastFailedCheckPoint = now;
            clearAnalysis();
            return false;
        }
        _analysisCheckPoint = now;
    }

    Trilian result = proofHang();
    if (result == Trilian::MAYBE) {
        return false;
    }
    if (result == Trilian::NO) {
        _lastFailedCheckPoint = now;
        clearAnalysis();
        return false;
    }

    return true;
}
