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

struct MetaLoopData {
    // References which of the loops this data applies to.
    // Range: [0, numLoops>
    int loopIndex;

    // References which of the run-blocks in the meta-loop this data applies to.
    // Range: [0, metaLoopPeriod>
    int runBlockIndex;

    // The number of instruction executed in the last loop iteration in case the loop was exited
    // prematurely.
    int loopRemainder = 0;

    // The number of iterations of the last analyzed run-block. It is a temporary value used for
    // setting and checking iterationDelta.
    int lastNumIterations = 0;

    // (An approximation of) the data pointer position of the last analyzed run-block. It is a
    // temporary value for setting dataPointerDelta.
    int dataPointerPos = 0;

    int lastIterationDelta = 0;
    bool isLinear = true;
    int dataPointerDelta = 0;

    MetaLoopData() {}
    MetaLoopData(int loopIndex, int runBlockIndex, int numIterations, int loopRemainder)
    : loopIndex(loopIndex)
    , runBlockIndex(runBlockIndex)
    , lastNumIterations(numIterations)
    , loopRemainder(loopRemainder) {}
};

class MetaLoopAnalysis;
class LoopBehavior {
    const MetaLoopAnalysis* _metaLoopAnalysis;
    // The index of the loop in the MetaLoopAnalysis
    int _loopIndex;

    std::shared_ptr<LoopAnalysis> _loopAnalysis;

    // How much the mininum (left) and maximum (right) DP value changes on subsequent executions of
    // the loop;
    int _minDpDelta;
    int _maxDpDelta;

    int _iterationDelta;

public:
    LoopBehavior(const MetaLoopAnalysis* metaLoopAnalysis, int loopIndex,
                 std::shared_ptr<LoopAnalysis> loopAnalysis,
                 int minDpDelta, int maxDpDelta, int iterationDelta)
    : _metaLoopAnalysis(metaLoopAnalysis)
    , _loopIndex(loopIndex)
    , _loopAnalysis(loopAnalysis)
    , _minDpDelta(minDpDelta)
    , _maxDpDelta(maxDpDelta)
    , _iterationDelta(iterationDelta) {}

    std::shared_ptr<LoopAnalysis> loopAnalysis() const { return _loopAnalysis; }

    int loopIndex() const { return _loopIndex; }
    int minDpDelta() const { return _minDpDelta; }
    int maxDpDelta() const { return _maxDpDelta; }
    int iterationDelta() const { return _iterationDelta; }

    const LoopBehavior& prevLoop() const;
    const LoopBehavior& nextLoop() const;

    LoopType loopType() const;
    bool isSweepLoop() const {
        LoopType tp = loopType();
        return tp == LoopType::ANCHORED_SWEEP || tp == LoopType::DOUBLE_SWEEP;
    }
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

    // The index of the run block from which this meta-loop analysis applies
    int _firstRunBlockIndex;
    // The number of meta-run blocks for which this meta-loop analysis is valid
    int _numMetaRunBlocks = 0;
    int _numRunBlocks = 0;

    // The number of loops and sequences in one meta runblock loop iteration. These sum to the
    // meta loop period.
    int _numLoops = 0;
    int _numSequences = 0;

    std::vector<std::shared_ptr<SequenceAnalysis>> _sequenceAnalysisPool;
    std::vector<std::shared_ptr<LoopAnalysis>> _loopAnalysisPool;

    // The analysis of every run block in the meta-loop (size = _metaLoopPeriod)
    std::vector<std::shared_ptr<SequenceAnalysis>> _seqAnalysis;

    // The properties of the loops in the meta-loop. Access by loop index.
    std::vector<MetaLoopData> _loopData;
    std::vector<LoopBehavior> _loopBehaviors;

    // Maps loop indices to sequence index.
    std::map<int, int> _loopIndexLookup;

    bool _isPeriodic;

    void analyzeRunBlocks(const ExecutionState &executionState);

    void initLoopData(const RunSummary &runSummary, int loopSize);
    bool checkLoopSize(const RunSummary &runSummary, int loopSize);
    bool determineLoopSize(const ExecutionState &executionState);

    void determineDpDeltas(const RunSummary &runSummary);
    void initLoopBehaviors();

public:
    void reset();

    // Should be invoked when a loop in the run summary is about to finish,
    bool analyzeMetaLoop(const ExecutionState &executionState);

    bool isAnalysisStillValid(const ExecutionState &executionState);

    // The meta-loop period (in run blocks). It is the period returned by the meta-run summary.
    int metaLoopPeriod() const { return _metaLoopPeriod; }

    // The loop size (in run blocks). It can be a multiple of the meta-loop period. This happens
    // when the number of iterations would otherwise increase non-linearly.
    int loopSize() const { return _loopSize; }

    // Returns true iff the program block history is periodic. This is the case when the number of
    // iterations for each loop inside the meta-loop remains constant.
    bool isPeriodic() const { return _isPeriodic; }

    // Returns the index of the first run block (with sequence index zero) from where this
    // meta-loop analysis is valid.
    int firstRunBlockIndex() const { return _firstRunBlockIndex; }

    // Returns the sequence index for the given loop.
    int sequenceIndexForLoop(int loopIndex) const { return _loopIndexLookup.at(loopIndex); }

    const std::vector<std::shared_ptr<SequenceAnalysis>> sequenceAnalysisResults() {
        return _seqAnalysis;
    }
    const std::vector<LoopBehavior>& loopBehaviors() const { return _loopBehaviors; }

    // Returns how much the number of iterations increased compared to previous invocation of the
    // loop run block in the meta-loop. In case the increase is non-linear, it returns the last
    // delta.
    //
    // Note: It differs from the iteration delta returned by the corresponding loop behavior when
    // loopSize != metaLoopPeriod.
    int loopIterationDelta(int loopIndex) const {
        return _loopData[loopIndex].lastIterationDelta;
    }

    // Returns how much the data pointer has shifted when execution of the run block starts
    // compared to previous invocation of the run block in the meta-loop.
    int dataPointerDelta(int loopIndex) const {
        return _loopData[loopIndex].dataPointerDelta;
    }

    int loopRemainder(int loopIndex) const {
        return _loopData[loopIndex].loopRemainder;
    }

    int loopRunBlockIndex(int loopIndex) const {
        return _loopData[loopIndex].runBlockIndex;
    }
};
