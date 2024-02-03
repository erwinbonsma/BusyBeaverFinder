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

class SweepHangChecker : public HangChecker {
public:
    enum class LocationInSweep : int8_t {
        UNSET = 0,
        LEFT,
        MID,
        RIGHT
    };

    // A transition where the sweep changes direction
    struct TransitionGroup {

        void analyze(const SweepHangChecker& checker, const ExecutionState& executionState);

        // When the sweep loops at both sides of the sweep are the same (e.g. when there is no
        // mid-sweep transition) the deltas in both sweep transition groups are equivalent.
        const DataDeltas& sweepLoopDeltas() const { return _sweepLoopDeltas; }

        const bool isStationary() const { return _isStationary; }
        const DataDeltas& transitionDeltas() const { return _transitionDeltas; }

      protected:
        LocationInSweep _location;
      private:
        DataDeltas _sweepLoopDeltas;
        DataDeltas _transitionDeltas;
        // Index of (one of) the incoming sweep-loop(s)
        int _incomingLoopSeqIndex;
        bool _isStationary;

        void initSweepLoopDeltas(const SweepHangChecker& checker, const RunSummary& runSummary);

        void analyzeTransition(const SweepHangChecker& checker,
                               const ExecutionState& executionState);

        friend class SweepHangChecker;
    };

    SweepHangChecker();

    bool init(const MetaLoopAnalysis* metaLoopAnalysis, const ExecutionState& executionState);

    const DataDeltas& sweepLoopDeltas(DataDirection dir) {
        return _transitionGroups[dir == DataDirection::RIGHT].sweepLoopDeltas();
    }

    // The direction indicates the side where the sweep changes direction.
    const TransitionGroup& sweepEndTransition(DataDirection dir) {
        return _transitionGroups[dir == DataDirection::RIGHT];
    }

    Trilian proofHang(const ExecutionState& executionState) override;

protected:
    static LocationInSweep opposite(LocationInSweep loc) {
        switch (loc) {
            case LocationInSweep::RIGHT: return LocationInSweep::LEFT;
            case LocationInSweep::LEFT: return LocationInSweep::RIGHT;
            default: assert(false);
        }
    }

    struct Location {
        LocationInSweep start {};
        LocationInSweep end {};

        bool isSweepLoop() const { return start != end; }
        bool isAt(LocationInSweep loc) const { return start == loc || end == loc; }
    };

    const Location& locationInSweep(int seqIndex) const { return _locationsInSweep.at(seqIndex); }
    const LoopBehavior& loopBehavior(int seqIndex) const {
        return _metaLoopAnalysis->loopBehaviors().at(
            _metaLoopAnalysis->loopIndexForSequence(seqIndex)
        );
    }

private:
    const MetaLoopAnalysis* _metaLoopAnalysis;

    // First is the group at the left, second is the group at the right
    std::array<TransitionGroup, 2> _transitionGroups;

    std::vector<Location> _locationsInSweep;
    // The sequence index of the first loop that is a sweep
    int _firstSweepLoopSeqIndex;

    // Set start and end locations for sweep loops
    bool locateSweepLoops();

    // Set the locations of the plain sequences and fixed-sized (non-sweep) loops
    void locateSweepTransitions();

    bool initTransitionSequences(const ExecutionState& executionState);
};
