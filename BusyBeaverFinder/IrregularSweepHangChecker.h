//
//  IrregularSweepHangChecker.hpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/05/2024.
//  Copyright © 2024 Erwin. All rights reserved.
//

#ifndef IrregularSweepHangChecker_h
#define IrregularSweepHangChecker_h

#include "SweepHangChecker.h"

class IrregularSweepHangChecker : public SweepHangChecker {
    struct EndProps {
        explicit EndProps(LocationInSweep location) : location(location) {}

        LocationInSweep location {};
    };

    // Used for irregular sweep-ends with a (growing) appendix.
    //
    // This includes the following behaviors:
    // - Appendices with a binary-like counter, where the sweep toggles values until the
    //   in-sweep exit (or the appendix end is reached)
    // - Appendices with a counter where the sweep exit only toggles the exit value and a
    //   neighouring value
    struct IrregularAppendixProps : public EndProps {
        explicit IrregularAppendixProps(LocationInSweep location) : EndProps(location) {}
        IrregularAppendixProps(const IrregularAppendixProps&) = default;

        // The value of the in-sweep exit
        int insweepExit {};

        // The value that is an in-sweep exit toggles to
        int insweepToggle {};

        // The delta that is applied to the values in the appendix. The following should hold:
        // insweepToggle + n * insweepDelta = insweepExit, with n > 0
        int insweepDelta {};

        // The DP where the appendix starts
        DataPointer appendixStart;
    };

    // Used for irregular sweeps that shrink the sequence at their end. This happens with this
    // behavior:
    // - The sweep shrinks when an in-sweep counter reaches zero. The start value of the next
    //   counter has a higher start value, which results in irregular shrinkage.
    struct ShrinkingEndProps : public EndProps {
        explicit ShrinkingEndProps(LocationInSweep location) : EndProps(location) {}
        ShrinkingEndProps(const ShrinkingEndProps&) = default;

        // The value that causes the sweep loop to exit
        int loopExit;

        // The sign of the counter value
        bool counterIsPositive;
    };

  public:
    virtual bool init(const MetaLoopAnalysis* metaLoopAnalysis,
                      const ExecutionState& executionState) override;

    bool isIrregular(DataDirection sweepEnd) const {
        return _endProps.find(mapDataDir(sweepEnd)) != _endProps.end();
    }
    bool isGrowing(DataDirection sweepEnd) const {
        return std::holds_alternative<IrregularAppendixProps>(_endProps.at(mapDataDir(sweepEnd)));
    }

    // The in-sweep exit is the value inside the sweep appendix that ends the sweep loop.
    int insweepExit(DataDirection sweepEnd) const {
        return std::get<IrregularAppendixProps>(_endProps.at(mapDataDir(sweepEnd))).insweepExit;
    }
    // The in-sweep toggle is the value inside the sweep appendix that does not cause the sweep to
    // exit, but is toggled to an exit value when it is traversed.
    //
    // Note: In complex programs there could be more than one toggle value, where toggle values
    // are also chained. In practise these do not occur for small programs so these are not (yet?)
    // supported.
    int insweepToggle(DataDirection sweepEnd) const {
        return std::get<IrregularAppendixProps>(_endProps.at(mapDataDir(sweepEnd))).insweepToggle;
    }

  protected:
    bool sweepLoopContinuesForever(const ExecutionState& executionState,
                                   SweepLoop* loop, int seqIndex) override;
    bool transitionContinuesForever(const ExecutionState& executionState,
                                    TransitionGroup* transition, int seqIndex) override;

    bool growingTransitionContinuesForever(const ExecutionState& executionState,
                                            TransitionGroup* transition, int seqIndex);
    bool shrinkingTransitionContinuesForever(const ExecutionState& executionState,
                                             TransitionGroup* transition, int seqIndex);
  private:
    LocationInSweep mapDataDir(DataDirection dir) const {
        return dir == DataDirection::LEFT ? LocationInSweep::LEFT : LocationInSweep::RIGHT;
    }

    bool checkMetaMetaLoop(const ExecutionState& executionState);
    bool findIrregularEnds();

    // Determines if there is a growing appendix and if it behaves as expected
    bool checkForAppendix(LocationInSweep location, const ExecutionState& executionState);

    bool checkForShrinkage(LocationInSweep location);

    // For irregular ends where with a (growing) appendix
    bool determineInSweepExits(IrregularAppendixProps& props);
    bool determineInSweepToggles(IrregularAppendixProps& props);
    bool determineAppendixStarts(IrregularAppendixProps& props,
                                 const ExecutionState& executionState);

    // For irregular ends that shrink.
    bool determineLoopExitValue(ShrinkingEndProps& props);
    bool determineCounterDelta(ShrinkingEndProps& props);

    std::vector<LocationInSweep> _irregularEnds;

    // Most of analysis supports case where both ends are irregular, so use a map to store
    // properties
    std::map<LocationInSweep, std::variant<IrregularAppendixProps, ShrinkingEndProps>> _endProps;

    // However, proof currently supports only one irregular end. Track it here.
    LocationInSweep _irregularEnd;

    bool _isInsideMetaMetaLoop;
};

#endif /* IrregularSweepHangChecker_hpp */
