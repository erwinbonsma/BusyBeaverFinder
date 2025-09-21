//
//  MetaLoopHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 20/11/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "MetaLoopHangDetector.h"

bool MetaLoopHangDetector::shouldCheckNow() const {
    auto loopRunState = _execution.getLoopRunState();
    return (// Only check at start and end of loops
            (loopRunState == LoopRunState::STARTED || loopRunState == LoopRunState::ENDED)
            // There should be a meta-loop
            && _execution.getMetaRunSummary().isInsideLoop()
            && (// The proof phase should also consider loop starts
                _activeChecker != nullptr
                // Perform analysis only when the inner-loop to finish
                || loopRunState == LoopRunState::ENDED));
}

void MetaLoopHangDetector::reset() {
    HangDetector::reset();

    _activeChecker = nullptr;
    _activeHang = HangType::UNKNOWN;
    _activeHangProofResult = Trilian::MAYBE;

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
        if (!_metaLoopAnalysis.isAnalysisStillValid(_execution)) {
            _metaLoopAnalysis.reset();
            _activeChecker = nullptr;
        }
    }

    // Redo meta-loop analysis if it is not valid, or there is no active checker. The latter may
    // happen when the meta-loop analysis did not (yet) latch onto the desired regularity.
    if (!_metaLoopAnalysis.isInitialized() || !_activeChecker) {
        if (!_metaLoopAnalysis.analyzeMetaLoop(_execution)) {
            return false;
        }
    }

    if (_activeChecker && _activeHangProofResult == Trilian::MAYBE) {
        // Preserve checker
        return true;
    }

//    _execution.dumpExecutionState();

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

        if (_activeHangProofResult == Trilian::NO) {
            _activeChecker = nullptr;
        }
    }

    return _activeHangProofResult;
}

void MetaLoopHangDetector::dump() const {
    std::cout << *this << std::endl;
}

std::ostream &operator<<(std::ostream &os, const MetaLoopHangDetector &hd) {
    os << "MetaLoopHangDetector, with loop analysis:" << std::endl;
    os << hd._loopAnalysis;
    os << "meta-loop analysis:" << std::endl;
    os << hd._metaLoopAnalysis;
    if (hd._activeChecker == &hd._irregularSweepHangChecker) {
        os << "irregular sweep hang checker:" << std::endl;
        os << hd._irregularSweepHangChecker;
    }

    return os;
}
