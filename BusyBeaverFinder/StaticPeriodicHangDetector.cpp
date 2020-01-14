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

bool StaticPeriodicHangDetector::canProofHang() {
    // TODO
    return false;
}
