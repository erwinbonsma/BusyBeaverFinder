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

void MetaLoopHangDetector::reset() {
    HangDetector::reset();

    _activeChecker = nullptr;
    _activeHang = HangType::UNKNOWN;
}

void MetaLoopHangDetector::clearAnalysis() {
    HangDetector::clearAnalysis();

    _metaLoopAnalysis.reset();
}

bool MetaLoopHangDetector::preparePeriodicHangCheck() {
    const RunSummary& runSummary = _execution.getRunSummary();

    // Loop period in run blocks
    int runBlocksInLoop = _metaLoopAnalysis.loopSize();

    int startRunBlockIndex = runSummary.getNumRunBlocks() - runBlocksInLoop;
    int endRunBlockIndex = runSummary.getNumRunBlocks() - 1;

    //    _metaLoopStart = metaLoopStart;
    int loopStart = runSummary.runBlockAt(startRunBlockIndex)->getStartIndex();
    int loopEnd = (runSummary.runBlockAt(endRunBlockIndex)->getStartIndex()
                   + runSummary.getRunBlockLength(endRunBlockIndex));
    // Loop period in program blocks
    int loopPeriod = loopEnd - loopStart;
    if (!_loopAnalysis.analyzeLoop(&_execution.getRunHistory()[loopStart], loopPeriod)) {
        return false;
    }

    _periodicHangChecker.init(&_loopAnalysis, loopStart);
    _activeChecker = &_periodicHangChecker;
    _activeHang = HangType::META_PERIODIC;

    return true;
}

bool MetaLoopHangDetector::prepareGliderHangCheck() {
    if (!_gliderHangChecker.init(&_metaLoopAnalysis, _execution)) {
        return false;
    }

    _activeChecker = &_gliderHangChecker;
    _activeHang = HangType::APERIODIC_GLIDER;

    return true;
}

bool MetaLoopHangDetector::analyzeHangBehaviour() {
    if (_metaLoopAnalysis.isAnalysisStillValid(_execution)) {
        return true;
    }

    if (!_metaLoopAnalysis.analyzeMetaLoop(_execution)) {
        return false;
    }

    if (_metaLoopAnalysis.isPeriodic()) {
        return preparePeriodicHangCheck();
    }

    if (prepareGliderHangCheck()) {
        return true;
    }

    // TODO: Remove once all types of loops can be checked
    _metaLoopAnalysis.reset();

    return false;
}

Trilian MetaLoopHangDetector::proofHang() {
    assert(_activeChecker != nullptr);

    Trilian result = _activeChecker->proofHang(_execution);
    if (result == Trilian::NO) {
        _activeChecker = nullptr;
        _activeHang = HangType::UNKNOWN;
    }

    return result;
}
