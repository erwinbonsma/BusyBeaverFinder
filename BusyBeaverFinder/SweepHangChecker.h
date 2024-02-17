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

        const DataDeltas& sweepLoopDeltas() const { return _analysis.dataDeltas(); }
        int deltaRange() const { return _deltaRange; }

        const SequenceAnalysis& sequenceAnalysis() const { return _analysis; }

      private:
        // Location should either be LEFT or RIGHT to uniquely identify the loop (as there are
        // two different loops arriving/departing from MID).
        LocationInSweep _location;

        // The effective result of the loop after one meta-loop period
        SequenceAnalysis _analysis;

        // The range of DP after which the sweep-loop behavior repeats. Only DP values in range of
        // [0, _deltaRange> should be considered when checking/proving hangs.
        int _deltaRange;

        // Index of (one of) the incoming sweep-loop(s)
        int _incomingLoopSeqIndex;

        void analyzeLoopAsSequence(const SweepHangChecker& checker,
                                   const ExecutionState& executionState);
    };

    // A transition where sweeps end and start
    class TransitionGroup {
      public:
        TransitionGroup(LocationInSweep loc) : _location(loc) {}

        void analyze(const SweepHangChecker& checker, const ExecutionState& executionState);

        const bool isStationary() const { return _isStationary; }
        const DataDeltas& transitionDeltas() const { return _analysis.squashedDataDeltas(); }

        const LoopAnalysis& loopAnalysis() const { return _analysis; }

      private:
        LocationInSweep _location;

        // The behavior over time.
        LoopAnalysis _analysis;
        // Index of (one of) the incoming sweep-loop(s)
        int _incomingLoopSeqIndex;
        bool _isStationary;

        void addSequenceInstructions(const SweepHangChecker& checker, const ExecutionState& state,
                                     int rbIndex, int dp);
        void addLoopInstructions(const SweepHangChecker& checker, const ExecutionState& state,
                                 int seqIndex, int rbIndex, int dp, bool incoming);
        void analyzeTransitionAsLoop(const SweepHangChecker& checker, const ExecutionState& state);
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

    // Returns sequence index for first incoming sweep loop of the given location
    int findIncomingSweepLoop(LocationInSweep location,
                              const ExecutionState& executionState) const;

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
