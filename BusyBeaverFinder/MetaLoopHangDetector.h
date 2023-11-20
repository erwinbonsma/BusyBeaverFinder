//
//  MetaLoopHangDetector.hpp
//  BusyBeaverFinder
//
//  Created by Erwin on 20/11/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include "HangDetector.h"

#include "MetaLoopAnalysis.h"

class MetaLoopHangDetector : public HangDetector {
    MetaLoopAnalysis _metaLoopAnalysis;
    HangType _detectedHang;

protected:
    bool shouldCheckNow(bool loopContinues) const override;
    bool analyzeHangBehaviour() override;
    Trilian proofHang() override;

public:
    MetaLoopHangDetector(const ExecutionState& execution) : HangDetector(execution) {}

    HangType hangType() const override { return _detectedHang; }
};
