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
  private:

    struct SweepLoopPart {
      public:
        SweepLoopPart(int seqIndex, int rbIndex, int dpOffset)
        : _seqIndex(seqIndex), _rbIndex(rbIndex), _dpOffset(dpOffset) {}

        int seqIndex() const { return _seqIndex; }
        int rbIndex() const { return _rbIndex; }
        int dpOffset() const { return _dpOffset; }
        int& seqIndex() { return _seqIndex; }
        int& rbIndex() { return _rbIndex; }
        int& dpOffset() { return _dpOffset; }

      private:
        // The index of the sequence within the sweep-loop
        int _seqIndex;
        // The index of the run block within the run-history
        int _rbIndex;
        // The DP when execution starts
        int _dpOffset;
    };

    struct SweepLoopVisitState {
        SweepLoopVisitState(const SweepHangChecker& checker,
                            const ExecutionState& executionState,
                            const SweepLoopPart& loopPart)
        : checker(checker), executionState(executionState), loopPart(loopPart) {}

        const SweepHangChecker& checker;
        const ExecutionState& executionState;
        const SweepLoopPart& loopPart;
    };

    using SweepLoopVisitor = std::function<void(const SweepLoopVisitState& state)>;

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
        int outgoingLoopSequenceIndex() const { return _outgoingLoopSeqIndex; }

        const LoopAnalysis& combinedAnalysis() const { return _analysis; }

        bool continuesForever(const ExecutionState& executionState, int dpDelta) const;

      private:
        // Location should either be LEFT or RIGHT to uniquely identify the loop (as there are
        // two different loops arriving/departing from MID).
        LocationInSweep _location;

        // The effective result of the loop after one meta-loop period
        LoopAnalysis _analysis;

        // The range of DP after which the sweep-loop behavior repeats. Only DP values in range of
        // [0, _deltaRange> should be considered when checking/proving hangs.
        int _deltaRange;

        // Index of (one of) the outgoing sweep-loop(s). This is the loop that is the starting
        // point for the analysis.
        int _outgoingLoopSeqIndex;

        void analyzeCombinedEffect(const SweepHangChecker& checker,
                                   const ExecutionState& executionState);
    };

    // A transition where sweeps end and start
    class TransitionGroup {
      public:
        TransitionGroup(LocationInSweep loc) : _location(loc) {}

        void analyze(const SweepHangChecker& checker, const ExecutionState& executionState);

        const bool isStationary() const { return _isStationary; }
        const DataDeltas& transitionDeltas() const { return _transitionDeltas; }

        int incomingLoopSequenceIndex() const { return _incomingLoopSeqIndex; }

        const LoopAnalysis& combinedAnalysis() const { return _analysis; }

        bool continuesForever(const ExecutionState& executionState) const;

      private:
        LocationInSweep _location;

        // The behavior over time.
        LoopAnalysis _analysis;
        // Index of (one of) the incoming sweep-loop(s)
        int _incomingLoopSeqIndex;
        bool _isStationary;
        int _minDp, _maxDp;
        DataDeltas _transitionDeltas;

        void addSequenceInstructions(const SweepLoopVisitState& vs);
        void addLoopInstructions(const SweepLoopVisitState& vs, bool incoming);
        void analyzeLoopPartPhase1(const SweepLoopVisitState& vs);
        void analyzeLoopPartPhase2(const SweepLoopVisitState& vs);
        void analyzeCombinedEffect(const SweepHangChecker& checker, const ExecutionState& state);
    };

    bool init(const MetaLoopAnalysis* metaLoopAnalysis, const ExecutionState& executionState);

    const SweepLoop& leftSweepLoop() const { return _leftSweepLoop; }
    const std::optional<SweepLoop>& rightSweepLoop() const { return _rightSweepLoop; }

    const TransitionGroup& leftTransition() const { return _leftTransition; }
    const TransitionGroup& rightTransition() const { return _rightTransition; }
    const std::optional<TransitionGroup>& midTransition() const { return _midTransition; }

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

    // Returns sequence index for first sweep loop satisfying the predicate (input Location for
    // the sweep loop)
    template <class Pred> int findSweepLoop(Pred p) const;

    // Add all contributions of the loop within the specified DP-range to the analysis
    //
    // Note: This may also include changes outside this range, as a continuous sequence of
    // instructions is replayed from the program's run history.
    void addContributionOfSweepLoopPass(const SweepHangChecker::SweepLoopVisitState vs,
                                        SequenceAnalysis& analysis, int minDp, int maxDp) const;
    void addContributionOfSweepLoopStart(const SweepHangChecker::SweepLoopVisitState vs,
                                         SequenceAnalysis& analysis, int minDp, int maxDp) const;
    void addContributionOfSweepLoopEnd(const SweepHangChecker::SweepLoopVisitState vs,
                                       SequenceAnalysis& analysis, int minDp, int maxDp) const;

    // Returns DP offset at end
    int visitSweepLoopParts(const SweepLoopVisitor& visitor, const ExecutionState& executionState,
                            int startSeqIndex, int dpStart) const;

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

    // How long (in total number of run blocks) to execute the proof checks before deciding the
    // program hangs
    int _proofUntil;

    // Set start and end locations for sweep loops
    bool locateSweepLoops();

    // Set the locations of the plain sequences and fixed-sized (non-sweep) loops
    void locateSweepTransitions();

    bool initSweepLoops(const ExecutionState& executionState);
    bool initTransitionSequences(const ExecutionState& executionState);
};

std::ostream &operator<<(std::ostream &os, const SweepHangChecker &checker);
