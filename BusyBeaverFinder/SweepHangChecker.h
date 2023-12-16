//
//  SweepHangChecker.h
//  BusyBeaverFinder
//
//  Created by Erwin on 16/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include <array>
#include <vector>

#include "HangChecker.h"
#include "LoopAnalysis.h"
#include "MetaLoopAnalysis.h"

namespace v2 {

struct SweepTransitionGroup {
    std::vector<const LoopBehavior*> incomingLoops;
    std::vector<const LoopBehavior*> outgoingLoops;
    SequenceAnalysis transitionSequence;
};

}

class SweepHangChecker : public HangChecker {
    const MetaLoopAnalysis* _metaLoopAnalysis;

    // First is the group at the left, second is the group at the right
    std::array<v2::SweepTransitionGroup, 2> _transitionGroups;

    bool extractSweepLoops();
    bool initTransitionSequences(const ExecutionState& executionState);
public:
    bool init(const MetaLoopAnalysis* metaLoopAnalysis, const ExecutionState& executionState);

    Trilian proofHang(const ExecutionState& executionState) override;
};
