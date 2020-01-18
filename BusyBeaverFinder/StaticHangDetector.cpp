//
//  StaticHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticHangDetector.h"

#include <cassert>

StaticHangDetector::StaticHangDetector(ExhaustiveSearcher& searcher) :
    _searcher(searcher)
{
    reset();
}

void StaticHangDetector::reset() {
    _lastCheckPoint = -1;
    _ongoingCheckPoint = -1;
    _stage = DetectionStage::WAIT_FOR_CHECKPOINT;
}

bool StaticHangDetector::detectHang(bool loopContinues) {
    int now = currentCheckPoint();
    bool resuming = _ongoingCheckPoint != -1;

    if (resuming && _ongoingCheckPoint != now) {
        _stage = DetectionStage::WAIT_FOR_CHECKPOINT;
        resuming = false;
        _ongoingCheckPoint = -1;
    }

    if (_stage == DetectionStage::WAIT_FOR_CHECKPOINT) {
        if (now == _lastCheckPoint) {
            return false;
        }
        _stage = DetectionStage::CHECK_BEHAVIOR;
    }

    if (_stage == DetectionStage::CHECK_BEHAVIOR) {
        Trilian result = exhibitsHangBehaviour(loopContinues);
        if (result == Trilian::MAYBE) {
            _ongoingCheckPoint = now;
            return false;
        }
        if (result == Trilian::NO) {
            _lastCheckPoint = now;
            _stage = DetectionStage::WAIT_FOR_CHECKPOINT;
            return false;
        }
        _stage = DetectionStage::VERIFY_HANG;
    }

    assert(_stage == DetectionStage::VERIFY_HANG);

    Trilian result = canProofHang(resuming);
    if (result == Trilian::MAYBE) {
        _ongoingCheckPoint = now;
        return false;
    }
    if (result == Trilian::NO) {
        _lastCheckPoint = now;
        _stage = DetectionStage::WAIT_FOR_CHECKPOINT;
        return false;
    }

    return true;
}
