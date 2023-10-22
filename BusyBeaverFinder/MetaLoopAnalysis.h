//
//  MetaLoopAnalysis.hpp
//  BusyBeaverFinder
//
//  Created by Erwin on 15/10/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include <vector>

#include "ExecutionState.h"
#include "SequenceAnalysis.h"
#include "LoopAnalysis.h"
#include "RunSummary.h"
#include "Types.h"

typedef const RunBlock *const * RawRunBlocks;

struct MetaLoopSequenceProps {
    int loopExit = 0;

    ValueChange dataPointerChange = ValueChange::NONE;
    ValueChange iterationChange = ValueChange::NONE;

    // The number of iterations of the last analyzed run-block (it is a temporary value used for
    // setting and checking iterationDelta)
    int numIterations = 0;

    // Deltas in case the change is linear (or none)
    int dataPointerDelta = 0;
    int iterationDelta = 0;

    MetaLoopSequenceProps() {}
    MetaLoopSequenceProps(int loopExit) : loopExit(loopExit) {}
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

    std::vector<SequenceAnalysis> _sequenceAnalysisPool;
    std::vector<LoopAnalysis> _loopAnalysisPool;

    // The analysis of every run block in the meta-loop (size = _metaLoopPeriod)
    std::vector<SequenceAnalysis *> _seqAnalysis;

    // The behaviour/properties of the run blocks in the meta-loop (size = _loopSize) 
    std::vector<MetaLoopSequenceProps> _seqProps;

    bool _isPeriodic;

    int _sequenceGrowth[numDataDirections];

    bool checkLoopSize(const RunSummary &runSummary, int loopSize);
    bool findLoopSize(const ExecutionState &executionState);

    void analyzeRunBlocks(const ExecutionState &executionState);

public:
    // Should be invoked when a loop in the run summary is about to finish,
    bool analyzeMetaLoop(const ExecutionState &executionState);

    // The loop size (in run blocks)
    int loopSize() { return _loopSize; }

    // Returns true iff the program block history is periodic. This is the case when the number of
    // iterations for each loop inside the meta-loop remains constant.
    bool isPeriodic() const { return _isPeriodic; }

    int loopIterationDelta(int runBlockIndex) const {
        return _seqProps[runBlockIndex].iterationDelta;
    }

    int sequenceGrowth(DataDirection dataDir) const { return _sequenceGrowth[(int)dataDir]; }
};
