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
}

bool StaticHangDetector::detectHang() {
    if (currentCheckPoint() != _lastCheckPoint) {
        if (exhibitsHangBehaviour()) {
            if (canProofHang()) {
                return true;
            }
        }
        _lastCheckPoint = currentCheckPoint();
    }
    return false;
}
