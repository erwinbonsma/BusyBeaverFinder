//
//  MetaPeriodicHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 18/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "MetaPeriodicHangDetector.h"

#include <limits>

#include "ExhaustiveSearcher.h"

MetaPeriodicHangDetector::MetaPeriodicHangDetector(ExhaustiveSearcher& searcher) :
    PeriodicHangDetector(searcher)
{
}

RunSummary* MetaPeriodicHangDetector::getTargetRunSummary() {
    return &_searcher.getMetaRunSummary();
}

bool MetaPeriodicHangDetector::insideLoop() {
    if (PeriodicHangDetector::insideLoop()) {
        // Set abort switch. This is required in case the assumed endless loop at meta-run level
        // is aborted and instead an endless loop at the lower level is entered. Without this
        // limit, the hang check itself would hang.
        _abortHangCheckAt = _searcher.getRunSummary().getNumProgramBlocks() * 3 / 2;
        return true;
    } else {
        return false;
    }
}

void MetaPeriodicHangDetector::start() {
    PeriodicHangDetector::start();

    _abortHangCheckAt = std::numeric_limits<int>::max();
}

HangDetectionResult MetaPeriodicHangDetector::detectHang() {
    if (_searcher.getRunSummary().getNumProgramBlocks() > _abortHangCheckAt) {
        return HangDetectionResult::FAILED;
    }
    else {
        return PeriodicHangDetector::detectHang();
    }
}
