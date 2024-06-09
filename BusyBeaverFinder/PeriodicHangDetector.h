//
//  PeriodicHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//
#pragma once

#include "HangDetector.h"

#include "ExhaustiveSearcher.h"
#include "RunSummary.h"
#include "LoopAnalysis.h"
#include "PeriodicHangChecker.h"

class PeriodicHangDetector : public HangDetector {

protected:
    LoopAnalysis _loop;
    PeriodicHangChecker _checker;

    bool shouldCheckNow() const override;

    // Analyses the loop. Returns YES if it exhibits periodic hang behavior. In that case, _loop
    // and _loopStart should point to the analyzed periodic loop and its starting point.
    bool analyzeHangBehaviour() override;

    Trilian proofHang() override;

public:
    PeriodicHangDetector(const ExecutionState& execution);

    HangType hangType() const override { return HangType::PERIODIC; }
};
