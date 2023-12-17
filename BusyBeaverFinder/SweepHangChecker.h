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

#include "DataDeltas.h"
#include "HangChecker.h"
#include "LoopAnalysis.h"
#include "MetaLoopAnalysis.h"

namespace v2 {

// A transition where the sweep changes direction
struct SweepTransitionGroup {

    // All sweep loops that arrive or depart at this transition point
    bool atRight;
    std::vector<const LoopBehavior*> sweepLoops;
    SequenceAnalysis transitionSequence;
    DataDeltas _sweepLoopDeltas;
    DataDeltas _stationaryTransitionDeltas;
    bool _isStationary;

    void analyze(const MetaLoopAnalysis* metaLoopAnalysis, const RunSummary& runSummary);

    const DataDeltas sweepLoopDeltas() const { return _sweepLoopDeltas; }

private:

    void initSweepLoopDeltas(const MetaLoopAnalysis* metaLoopAnalysis,
                             const RunSummary& runSummary);
    void analyzeStationaryTransition(const MetaLoopAnalysis* metaLoopAnalysis,
                                     const RunSummary& runSummary);
    void analyzeGliderTransition();
};

}

class SweepHangChecker : public HangChecker {
    const MetaLoopAnalysis* _metaLoopAnalysis;

    // First is the group at the left, second is the group at the right
    std::array<v2::SweepTransitionGroup, 2> _transitionGroups;

    bool extractSweepLoops();
    bool initTransitionSequences(const ExecutionState& executionState);
public:
    SweepHangChecker();

    bool init(const MetaLoopAnalysis* metaLoopAnalysis, const ExecutionState& executionState);

    // The direction indicates the side where the sweep changes direction. When the sweep loops at
    // both sides are the same (e.g. when there is no mid-sweep transition) the deltas at both
    // sides are equivalent.
    const DataDeltas sweepLoopDeltas(DataDirection dir) {
        return _transitionGroups[dir == DataDirection::RIGHT]._sweepLoopDeltas;
    }

    Trilian proofHang(const ExecutionState& executionState) override;
};
