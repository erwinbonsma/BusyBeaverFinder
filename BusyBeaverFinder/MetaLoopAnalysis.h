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

typedef const RunBlock *const * RawRunBlocks;

/* Analyses a meta-run loop. The loops contained in the meta-run loop should either have a fixed
 * number of iterations each iteration of the meta-run loop, or increase at a constant rate.
 */
class MetaLoopAnalysis {
    std::vector<SequenceAnalysis> _sequenceAnalysisPool;
    std::vector<LoopAnalysis> _loopAnalysisPool;
    std::vector<SequenceAnalysis *> _sequenceAnalysis;

    // The size of the meta-loop in run blocks. This is a multiple of the meta-loop period.
    //
    // Consider this run history: A[2] B A[2] B A[3] B A[3] B A[4] B A[4]
    // This results in the following meta run history loop: A[n] B consisting of two run blocks.
    // However, two iterations need to be combined for the loop to increase at a constant rate.
    // The analyzed meta loop therefore would consist of four run blocks (A[n] B A[n] B).
    int _loopSize;

    // The increase in the number of iterations for each loop inside the meta-run loop. For
    // simplicity, sequences are treated as loops with iteration zero.
    std::vector<int> _iterationDeltas;
    bool _isPeriodic;

    int _sequenceGrowth[numDataDirections];

    bool establishIterationDeltas(const RunSummary &runSummary, int start, int end);
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

    int loopIterationDelta(int runBlockIndex) const { return _iterationDeltas[runBlockIndex]; }

    int sequenceGrowth(DataDirection dataDir) const { return _sequenceGrowth[(int)dataDir]; }
};
