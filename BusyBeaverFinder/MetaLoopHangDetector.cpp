//
//  MetaLoopHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 20/11/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "MetaLoopHangDetector.h"

bool MetaLoopHangDetector::shouldCheckNow(bool loopContinues) const {
    // Should wait for the inner-loop to finish
    return !loopContinues && _execution.getMetaRunSummary().isInsideLoop();
}

bool MetaLoopHangDetector::analyzeHangBehaviour() {
    return _metaLoopAnalysis.analyzeMetaLoop(_execution);
}

Trilian MetaLoopHangDetector::proofHang() {
    if (_metaLoopAnalysis.isPeriodic()) {
        // TODO: Implement logic from meta-periodic hang detector
    }

    return Trilian::MAYBE;
}
