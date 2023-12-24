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

    bool atRight;
    // All sweep loops that arrive or depart at this transition point
    std::vector<const LoopBehavior*> sweepLoops;

    void analyze(const MetaLoopAnalysis* metaLoopAnalysis, const ExecutionState& executionState);

    // When the sweep loops at both sides of the sweep are the same (e.g. when there is no
    // mid-sweep transition) the deltas in both sweep transition groups are equivalent.
    const DataDeltas& sweepLoopDeltas() const { return _sweepLoopDeltas; }

    const bool isStationary() const { return _isStationary; }
    const DataDeltas& stationaryTransitionDeltas() const { return _stationaryTransitionDeltas; }

private:
    SequenceAnalysis transitionSequence;
    DataDeltas _sweepLoopDeltas;
    DataDeltas _stationaryTransitionDeltas;
    bool _isStationary;

    struct Bounds {
        int minDp = 0;
        int maxDp = 0;
    };

    void initSweepLoopDeltas(const MetaLoopAnalysis* metaLoopAnalysis,
                             const RunSummary& runSummary);

    void analyzeStationaryTransition(const MetaLoopAnalysis* metaLoopAnalysis,
                                     const ExecutionState& executionState);
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

    const DataDeltas& sweepLoopDeltas(DataDirection dir) {
        return _transitionGroups[dir == DataDirection::RIGHT].sweepLoopDeltas();
    }

    // The direction indicates the side where the sweep changes direction.
    const v2::SweepTransitionGroup& sweepEndTransition(DataDirection dir) {
        return _transitionGroups[dir == DataDirection::RIGHT];
    }

    Trilian proofHang(const ExecutionState& executionState) override;
};
