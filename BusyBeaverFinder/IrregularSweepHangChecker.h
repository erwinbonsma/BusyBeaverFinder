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
  public:
    virtual bool init(const MetaLoopAnalysis* metaLoopAnalysis,
                      const ExecutionState& executionState) override;

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

  protected:
    bool sweepLoopContinuesForever(const ExecutionState& executionState,
                                   SweepLoop* loop, int seqIndex) override;
    bool transitionContinuesForever(const ExecutionState& executionState,
                                    TransitionGroup* transition, int seqIndex) override;

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

    // Most of analysis supports case where both ends are irregular, so use a map to store
    // properties
    std::map<LocationInSweep, IrregularEndProps> _endProps;

    // However, proof currently supports only one irregular end. Track it here.
    LocationInSweep _irregularEnd;
};

#endif /* IrregularSweepHangChecker_hpp */
