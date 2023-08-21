//
//  MetaPeriodicHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 18/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//
#pragma once

#include "PeriodicHangDetector.h"

class MetaPeriodicHangDetector : public PeriodicHangDetector {
    int _metaLoopStart;
    bool _lastAnalysisResult;

protected:
    bool shouldCheckNow(bool loopContinues) const override;
    bool analyzeHangBehaviour() override;
    Trilian proofHang() override;

public:
    MetaPeriodicHangDetector(const ExecutionState& execution);

    HangType hangType() const override { return HangType::META_PERIODIC; }

    void reset() override;
};
