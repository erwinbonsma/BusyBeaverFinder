//
//  PeriodicHangChecker.h
//  BusyBeaverFinder
//
//  Created by Erwin on 27/11/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include "HangChecker.h"
#include "LoopAnalysis.h"

class PeriodicHangChecker : public HangChecker {

    const LoopAnalysis* _loop;
    size_t _loopStart;

    int _proofPhase;
    size_t _targetLoopLen;

protected:
    Trilian proofHangPhase1(const ExecutionState& _execution);
    Trilian proofHangPhase2(const ExecutionState& _execution);

public:
    void init(const LoopAnalysis* loop, size_t loopStart);

    size_t loopStart() const { return _loopStart; }

    Trilian proofHang(const ExecutionState& _execution) override;
};
