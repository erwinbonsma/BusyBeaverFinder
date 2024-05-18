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

  protected:
    struct SweepLoopVisitState {
        SweepLoopVisitState(const SweepHangChecker& checker,
                            const ExecutionState& executionState,
                            const SweepLoopPart& loopPart)
        : checker(checker), executionState(executionState), loopPart(loopPart) {}

        const SweepHangChecker& checker;
        const ExecutionState& executionState;
        const SweepLoopPart& loopPart;
    };

    using SweepLoopVisitor = std::function<bool(const SweepLoopVisitState& state)>;

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

        void reset() {
            _incomingLoops.clear();
            _outgoingLoops.clear();
        }
        void addIncomingLoop(std::shared_ptr<LoopAnalysis> loop) {
            _incomingLoops.insert(loop);
        }
        void addOutgoingLoop(std::shared_ptr<LoopAnalysis> loop) {
            _outgoingLoops.insert(loop);
        }
        const std::set<std::shared_ptr<LoopAnalysis>>& incomingLoops() const {
            return _incomingLoops;
        }
        const std::set<std::shared_ptr<LoopAnalysis>>& outgoingLoops() const {
            return _outgoingLoops;
        }

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

        // The analysis of incoming and outgoing loops. Only one entry for each unique loop
        std::set<std::shared_ptr<LoopAnalysis>> _incomingLoops;
        std::set<std::shared_ptr<LoopAnalysis>> _outgoingLoops;

        void analyzeCombinedEffect(const SweepHangChecker& checker,
                                   const ExecutionState& executionState);
    };

    // A transition where sweeps end and start
    class TransitionGroup {
      public:
        TransitionGroup(LocationInSweep loc) : _location(loc) {}

        bool analyze(const SweepHangChecker& checker, const ExecutionState& executionState);

        bool isStationary() const { return _isStationary; }
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
        bool analyzeLoopPartPhase2(const SweepLoopVisitState& vs);
        bool analyzeCombinedEffect(const SweepHangChecker& checker, const ExecutionState& state);
    };

    virtual bool init(const MetaLoopAnalysis* metaLoopAnalysis,
                      const ExecutionState& executionState);

    const SweepLoop& leftSweepLoop() const { return _leftSweepLoop; }
    const std::optional<SweepLoop>& rightSweepLoop() const { return _rightSweepLoop; }

    const TransitionGroup& leftTransition() const { return _leftTransition; }
    const TransitionGroup& rightTransition() const { return _rightTransition; }
    const std::optional<TransitionGroup>& midTransition() const { return _midTransition; }

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
    bool addContributionOfSweepLoopEnd(const SweepHangChecker::SweepLoopVisitState vs,
                                       SequenceAnalysis& analysis, int minDp, int maxDp) const;

    // When successful returns DP offset at end
    std::optional<int> visitSweepLoopParts(const SweepLoopVisitor& visitor,
                                           const ExecutionState& executionState,
                                           int startSeqIndex, int dpStart,
                                           int numLoopIterations = 1) const;

    const Location& locationInSweep(int seqIndex) const { return _locationsInSweep.at(seqIndex); }
    const LoopBehavior& loopBehavior(int seqIndex) const {
        return _metaLoopAnalysis->loopBehaviors().at(
            _metaLoopAnalysis->loopIndexForSequence(seqIndex)
        );
    }

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

  private:
    // Set start and end locations for sweep loops
    bool locateSweepLoops();

    // Set the locations of the plain sequences and fixed-sized (non-sweep) loops
    void locateSweepTransitions();

    bool initSweepLoops(const ExecutionState& executionState);
    bool initTransitionSequences(const ExecutionState& executionState);
};

class RegularSweepHangChecker : public SweepHangChecker {
  public:
    virtual bool init(const MetaLoopAnalysis* metaLoopAnalysis,
                      const ExecutionState& executionState) override;

    Trilian proofHang(const ExecutionState& executionState) override;

  private:
    // How long (in total number of run blocks) to execute the proof checks before deciding the
    // program hangs
    int _proofUntil;
};

class IrregularSweepHangChecker : public SweepHangChecker {
  public:
    virtual bool init(const MetaLoopAnalysis* metaLoopAnalysis,
                      const ExecutionState& executionState) override;

    Trilian proofHang(const ExecutionState& executionState) override;

    bool isIrregular(DataDirection sweepEnd) const {
        return _endProps.find(mapDataDir(sweepEnd)) != _endProps.end();
    }
    // The in-sweep exit is the value inside the sweep appendix that ends the sweep loop.
    int insweepExit(DataDirection sweepEnd) const {
        return _endProps.at(mapDataDir(sweepEnd)).insweepExit;
    }
    // The in-sweep toggle is the value inside the sweep appendix that does not cause the sweep to
    // exit, but is toggled to an exit value when it is traversed.
    //
    // Note: In complex programs there could be more than one toggle value, where toggle values
    // are also chained. In practise these do not occur for small programs so these are not (yet?)
    // supported.
    int insweepToggle(DataDirection sweepEnd) const {
        return _endProps.at(mapDataDir(sweepEnd)).insweepToggle;
    }

  private:
    LocationInSweep mapDataDir(DataDirection dir) const {
        return dir == DataDirection::LEFT ? LocationInSweep::LEFT : LocationInSweep::RIGHT;
    }

    struct IrregularEndProps {
        // The value of the in-sweep exit
        int insweepExit {};

        // The value that is toggled to an in-sweep exit
        int insweepToggle {};

        // The DP where the appendix starts
        DataPointer appendixStart;
    };

    bool checkMetaMetaLoop(const ExecutionState& executionState);
    bool findIrregularEnds();
    bool determineInSweepExits();
    bool determineInSweepToggles();
    bool determineAppendixStarts(const ExecutionState& executionState);

    std::map<LocationInSweep, IrregularEndProps> _endProps;
};

std::ostream &operator<<(std::ostream &os, const SweepHangChecker &checker);
