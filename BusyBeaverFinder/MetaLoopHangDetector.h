//
//  MetaLoopHangDetector.hpp
//  BusyBeaverFinder
//
//  Created by Erwin on 20/11/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include "HangDetector.h"

#include "GliderHangChecker.h"
#include "HangChecker.h"
#include "MetaLoopAnalysis.h"
#include "PeriodicHangChecker.h"
#include "SweepHangChecker.h"
#include "IrregularSweepHangChecker.h"

class MetaLoopHangDetector : public HangDetector {
    MetaLoopAnalysis _metaLoopAnalysis;
    PeriodicHangChecker _periodicHangChecker;
    GliderHangChecker _gliderHangChecker;
    SweepHangChecker _regularSweepHangChecker;
    IrregularSweepHangChecker _irregularSweepHangChecker;
    LoopAnalysis _loopAnalysis;

    HangChecker* _activeChecker;
    HangType _activeHang;
    Trilian _activeHangProofResult;

    bool preparePeriodicHangCheck();
    bool prepareGliderHangCheck();
    bool prepareSweepHangCheck();
    bool prepareIrregularSweepHangCheck();

  protected:
    bool shouldCheckNow() const override;
    bool analyzeHangBehaviour() override;
    Trilian proofHang() override;

  public:
    MetaLoopHangDetector(const ExecutionState& execution) : HangDetector(execution) {}

    void reset() override;
    HangType hangType() const override { return _activeHang; }
    const MetaLoopAnalysis& metaLoopAnalysis() { return _metaLoopAnalysis; };
};
