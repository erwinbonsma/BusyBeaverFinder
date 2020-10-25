//
//  StaticHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticHangDetector.h"

#include <cassert>
#include <iostream>

StaticHangDetector::StaticHangDetector(const ProgramExecutor& executor) :
    _executor(executor)
{
    reset();
}

void StaticHangDetector::reset() {
    _lastCheckPoint = -1;
    _analysisCheckPoint = -1;
}

bool StaticHangDetector::detectHang(bool loopContinues) {
    if (!shouldCheckNow(loopContinues)) {
        return false;
    }

    int now = currentCheckPoint() + !loopContinues;

    if (now == _lastCheckPoint) {
        // We already checked this and it failed. Ignore.
        return false;
    }

    if (_analysisCheckPoint != now) {
        // We have not yet analysed the current situation
        if (!analyzeHangBehaviour()) {
            _lastCheckPoint = now;
            return false;
        }
        _analysisCheckPoint = now;
    }

    Trilian result = proofHang();
    if (result == Trilian::MAYBE) {
        return false;
    }
    if (result == Trilian::NO) {
        _lastCheckPoint = now;
        return false;
    }

    return true;
}
