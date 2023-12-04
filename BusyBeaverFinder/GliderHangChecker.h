//
//  GliderHangChecker.h
//  BusyBeaverFinder
//
//  Created by Erwin on 03/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include "HangChecker.h"
#include "MetaLoopAnalysis.h"

class GliderHangChecker : public HangChecker {
    const MetaLoopAnalysis* _metaLoopAnalysis;
    int _gliderLoopIndex;
    int _curCounterDpOffset;
    int _nxtCounterDpOffset;

    bool identifyLoopCounters();

public:
    bool init(const MetaLoopAnalysis* metaLoopAnalysis);

    Trilian proofHang(const ExecutionState& _execution) override;
};
