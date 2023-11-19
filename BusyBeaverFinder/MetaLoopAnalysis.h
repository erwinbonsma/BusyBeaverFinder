//
//  MetaLoopAnalysis.hpp
//  BusyBeaverFinder
//
//  Created by Erwin on 15/10/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include <memory>
#include <vector>

#include "ExecutionState.h"
#include "SequenceAnalysis.h"
#include "LoopAnalysis.h"
#include "RunSummary.h"
#include "Types.h"

typedef const RunBlock *const * RawRunBlocks;

struct MetaLoopSequenceProps {
    // The number of instruction executed in the last loop iteration in case the loop was exited
    // prematurely.
    int loopRemainder = 0;

    // The number of iterations of the last analyzed run-block. It is a temporary value used for
    // setting and checking iterationDelta.
    int numIterations = 0;

    // (An approximation of) the data pointer position of the last analyzed run-block. It is a
    // temporary value for setting dataPointerDelta.
    int dataPointerPos = 0;

    // Deltas in case the change is linear (or none)
    int iterationDelta = 0;
    int dataPointerDelta = 0;

    MetaLoopSequenceProps() {}
    MetaLoopSequenceProps(int loopRemainder) : loopRemainder(loopRemainder) {}
};

class LoopBehavior {
    std::shared_ptr<LoopAnalysis> _loopAnalysis;
    // How much the mininum (left) and maximum (right) DP value changes on subsequent executions of
    // the loop;
    int _minDpDelta;
    int _maxDpDelta;
    int _iterationDelta;
public:
    LoopBehavior(std::shared_ptr<LoopAnalysis> loopAnalysis,
                 int minDpDelta, int maxDpDelta, int iterationDelta)
    : _loopAnalysis(loopAnalysis), _minDpDelta(minDpDelta), _maxDpDelta(maxDpDelta),
    _iterationDelta(iterationDelta) {}

    std::shared_ptr<LoopAnalysis> loopAnalysis() { return _loopAnalysis; }

    int minDpDelta() const { return _minDpDelta; }
    int maxDpDelta() const { return _maxDpDelta; }
    int iterationDelta() const { return _iterationDelta; }
    LoopType loopType() const;
};

/* Analyses a meta-run loop.
 *
 * The following conditions need to hold:
 * - The loops contained in the meta-run loop should always exit at the same program block.
 * - The DP position at the start of each run block every meta-iteration should be fixed, or move
 *   at constant speed, which implies
 *   - The number of iterations for non-stationairy loops should be constant or increase linearly
 */
class MetaLoopAnalysis {
    // The meta-loop period in run blocks.
    int _metaLoopPeriod;

    // The size of the meta-loop in run blocks. This could be a multiple of the meta-loop period
    // to satisfy the meta-run loop conditions
    int _loopSize;

    std::vector<std::shared_ptr<SequenceAnalysis>> _sequenceAnalysisPool;
    std::vector<std::shared_ptr<LoopAnalysis>> _loopAnalysisPool;

    // The analysis of every run block in the meta-loop (size = _metaLoopPeriod)
    std::vector<std::shared_ptr<SequenceAnalysis>> _seqAnalysis;

    // The behaviour/properties of the run blocks in the meta-loop (size = _loopSize) 
    std::vector<MetaLoopSequenceProps> _seqProps;

    std::vector<LoopBehavior> _loopBehaviors;

    bool _isPeriodic;

    int _sequenceGrowth[numDataDirections];

    void analyzeRunBlocks(const ExecutionState &executionState);

    bool checkLoopSize(const RunSummary &runSummary, int loopSize);
    bool determineLoopSize(const ExecutionState &executionState);

    void determineDpDeltas(const RunSummary &runSummary);
    void initLoopBehaviors();

public:
    // Should be invoked when a loop in the run summary is about to finish,
    bool analyzeMetaLoop(const ExecutionState &executionState);

    // The meta-loop period (in run blocks). It is the period returned by the meta-run summary.
    int metaLoopPeriod() const { return _metaLoopPeriod; }

    // The loop size (in run blocks). It can be a multiple of the meta-loop period. This happens
    // when the number of iterations would otherwise increase non-linearly.
    int loopSize() const { return _loopSize; }

    // Returns true iff the program block history is periodic. This is the case when the number of
    // iterations for each loop inside the meta-loop remains constant.
    bool isPeriodic() const { return _isPeriodic; }

    const std::vector<LoopBehavior>& loopBehaviors() const { return _loopBehaviors; }

    // Returns how much the number of iterations increased compared to previous invocation of the
    // run block in the meta-loop.
    //
    // Note: It differs from the iteration delta returned by the corresponding loop behavior when
    // loopSize != metaLoopPeriod.
    int loopIterationDelta(int runBlockIndex) const {
        return _seqProps[runBlockIndex].iterationDelta;
    }

    // Returns how much the data pointer has shifted when execution of the run block starts
    // compared to previous invocation of the run block in the meta-loop.
    int dataPointerDelta(int runBlockIndex) const {
        return _seqProps[runBlockIndex].dataPointerDelta;
    }

    int sequenceGrowth(DataDirection dataDir) const { return _sequenceGrowth[(int)dataDir]; }
};
