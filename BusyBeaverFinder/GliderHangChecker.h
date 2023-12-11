//
//  GliderHangChecker.h
//  BusyBeaverFinder
//
//  Created by Erwin on 03/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include "HangChecker.h"
#include "LoopAnalysis.h"
#include "MetaLoopAnalysis.h"

class GliderHangChecker : public HangChecker {
    const MetaLoopAnalysis* _metaLoopAnalysis;

    // The loop analysis of the transition sequence. Although the transition itself is a sequence,
    // the loop analysis shows the effects over repeated iterations of the meta-loop.
    LoopAnalysis _transitionLoopAnalysis;

    // The index of the loop in the meta-loop analysis (zero when there's only one loop)
    int _gliderLoopIndex;

    // The instruction in the glider loop that decrements the loop counter (which eventually causes
    // the loop to exit)
    int _loopCounterIndex;

    // The location of the (current) loop counter, relative to DP at loop entry.
    int _counterDpOffset;

    bool identifyLoopCounters();
    bool analyzeTransitionSequence(const ExecutionState& executionState);

public:
    bool init(const MetaLoopAnalysis* metaLoopAnalysis, const ExecutionState& executionState);

    int gliderLoopIndex() const { return _gliderLoopIndex; }
    int counterDpOffset() const { return _counterDpOffset; }
    const LoopAnalysis transitionLoopAnalysis() const { return _transitionLoopAnalysis; }

    Trilian proofHang(const ExecutionState& executionState) override;
};
