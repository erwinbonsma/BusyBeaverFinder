//
//  SweepHangChecker.h
//  BusyBeaverFinder
//
//  Created by Erwin on 16/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include <array>
#include <optional>
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

    class SweepLoop {
      public:
        SweepLoop(LocationInSweep loc) : _location(loc) {}

        void analyze(const SweepHangChecker& checker, const ExecutionState& executionState);

        // When the sweep loops at both sides of the sweep are the same (e.g. when there is no
        // mid-sweep transition) the deltas in both sweep transition groups are equivalent.
        const DataDeltas& sweepLoopDeltas() const { return _sweepLoopDeltas; }

      private:
        // Location should either be LEFT or RIGHT to uniquely identify the loop (as there are
        // two different loops arriving/departing from MID).
        LocationInSweep _location;

        DataDeltas _sweepLoopDeltas;
        // Index of (one of) the incoming sweep-loop(s)
        int _incomingLoopSeqIndex;

        void initSweepLoopDeltas(const SweepHangChecker& checker, const RunSummary& runSummary);
    };

    // A transition where sweeps end and start
    class TransitionGroup {
      public:
        TransitionGroup(LocationInSweep loc) : _location(loc) {}

        void analyze(const SweepHangChecker& checker, const ExecutionState& executionState);

        const bool isStationary() const { return _isStationary; }
        const DataDeltas& transitionDeltas() const { return _transitionDeltas; }

      private:
        LocationInSweep _location;
        DataDeltas _transitionDeltas;
        // Index of (one of) the incoming sweep-loop(s)
        int _incomingLoopSeqIndex;
        bool _isStationary;

        void addDeltasFromTransitions(const SweepHangChecker& checker,
                                      const ExecutionState& executionState);
        void addDeltasFromSweepLoops(const SweepHangChecker& checker,
                                     const ExecutionState& executionState);
    };

    bool init(const MetaLoopAnalysis* metaLoopAnalysis, const ExecutionState& executionState);

    const SweepLoop& leftSweepLoop() { return _leftSweepLoop; }
    const std::optional<SweepLoop>& rightSweepLoop() { return _rightSweepLoop; }

    const TransitionGroup& leftTransition() { return _leftTransition; }
    const TransitionGroup& rightTransition() { return _rightTransition; }
    const std::optional<TransitionGroup>& midTransition() { return _midTransition; }

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

    TransitionGroup _leftTransition { LocationInSweep::LEFT };
    TransitionGroup _rightTransition { LocationInSweep::RIGHT };
    std::optional<TransitionGroup> _midTransition {};

    SweepLoop _leftSweepLoop { LocationInSweep::LEFT };
    // Only set when there is a mid-sweep transition
    std::optional<SweepLoop> _rightSweepLoop;

    std::vector<Location> _locationsInSweep;
    // The sequence index of the first loop that is a sweep
    int _firstSweepLoopSeqIndex;

    // Set start and end locations for sweep loops
    bool locateSweepLoops();

    // Set the locations of the plain sequences and fixed-sized (non-sweep) loops
    void locateSweepTransitions();

    bool initSweepLoops(const ExecutionState& executionState);
    bool initTransitionSequences(const ExecutionState& executionState);
};
