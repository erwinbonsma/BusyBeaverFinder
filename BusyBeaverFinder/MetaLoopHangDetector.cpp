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
    _activeHangProofResult = Trilian::MAYBE;

    clearAnalysis();
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
    _activeHangProofResult = Trilian::MAYBE;

    return true;
}

bool MetaLoopHangDetector::prepareGliderHangCheck() {
    if (!_gliderHangChecker.init(&_metaLoopAnalysis, _execution)) {
        return false;
    }

    _activeChecker = &_gliderHangChecker;
    _activeHang = HangType::APERIODIC_GLIDER;
    _activeHangProofResult = Trilian::MAYBE;

    return true;
}

bool MetaLoopHangDetector::prepareSweepHangCheck() {
    if (!_regularSweepHangChecker.init(&_metaLoopAnalysis, _execution)) {
        return false;
    }

    _activeChecker = &_regularSweepHangChecker;
    _activeHang = HangType::REGULAR_SWEEP;
    _activeHangProofResult = Trilian::MAYBE;

    return true;
}

bool MetaLoopHangDetector::prepareIrregularSweepHangCheck() {
    if (!_irregularSweepHangChecker.init(&_metaLoopAnalysis, _execution)) {
        return false;
    }

    _activeChecker = &_irregularSweepHangChecker;
    _activeHang = HangType::IRREGULAR_SWEEP;
    _activeHangProofResult = Trilian::MAYBE;

    return true;
}

bool MetaLoopHangDetector::analyzeHangBehaviour() {
    if (_metaLoopAnalysis.isInitialized()) {
        if (_metaLoopAnalysis.isAnalysisStillValid(_execution)) {
            return true;
        } else {
            // Clear any checker that is no longer valid
            _activeChecker = nullptr;
        }
    }

    if (!_metaLoopAnalysis.analyzeMetaLoop(_execution)) {
        return false;
    }

    if (_metaLoopAnalysis.isPeriodic()) {
        return preparePeriodicHangCheck();
    }

    if (_metaLoopAnalysis.isRegular()) {
        return prepareGliderHangCheck() || prepareSweepHangCheck();
    }

    return prepareIrregularSweepHangCheck();

    // TODO: Check which programs are not covered by the existing checkers
    return false;
}

Trilian MetaLoopHangDetector::proofHang() {
    assert(_activeChecker != nullptr);

    if (_activeHangProofResult == Trilian::MAYBE) {
        _activeHangProofResult = _activeChecker->proofHang(_execution);
    }

    return _activeHangProofResult;
}
