//
//  MetaLoopHangDetector.hpp
//  BusyBeaverFinder
//
//  Created by Erwin on 20/11/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include "HangDetector.h"

#include "HangChecker.h"
#include "MetaLoopAnalysis.h"
#include "PeriodicHangChecker.h"

class MetaLoopHangDetector : public HangDetector {
    MetaLoopAnalysis _metaLoopAnalysis;
    PeriodicHangChecker  _periodicHangChecker;
    LoopAnalysis _loopAnalysis;

    HangChecker* _activeChecker;
    HangType _activeHang;

    bool preparePeriodicHangCheck();

protected:
    void clearAnalysis() override;
    bool shouldCheckNow(bool loopContinues) const override;
    bool analyzeHangBehaviour() override;
    Trilian proofHang() override;

public:
    MetaLoopHangDetector(const ExecutionState& execution) : HangDetector(execution) {}

    void reset() override;
    HangType hangType() const override { return _activeHang; }
};
