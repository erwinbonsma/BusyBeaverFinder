//
//  StaticHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticHangDetector.h"

StaticHangDetector::StaticHangDetector(ExhaustiveSearcher& searcher) :
    _searcher(searcher)
{
    _lastCheckPoint = -1;
    _ongoingCheckPoint = -1;
}

bool StaticHangDetector::detectHang() {
    bool resumed = (currentCheckPoint() == _ongoingCheckPoint);

    if (resumed || currentCheckPoint() != _lastCheckPoint) {
        if (resumed || exhibitsHangBehaviour()) {
            HangDetectionResult result = tryProofHang(resumed);
            switch (result) {
                case HangDetectionResult::HANGING:
                    return true;
                case HangDetectionResult::ONGOING:
                    _ongoingCheckPoint = currentCheckPoint();
                    break;
                default:
                    _ongoingCheckPoint = -1;
            }
        }
        _lastCheckPoint = currentCheckPoint();
    }

    // Could not detect hang (yet)
    return false;
}
