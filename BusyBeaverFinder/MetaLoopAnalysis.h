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
    // Range: [0, metaLoop.loopSize>
    int sequenceIndex;

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
    MetaLoopData(int loopIndex, int sequenceIndex, int numIterations, int loopRemainder)
    : loopIndex(loopIndex)
    , sequenceIndex(sequenceIndex)
    , lastNumIterations(numIterations)
    , loopRemainder(loopRemainder) {}
};

class MetaLoopAnalysis;

// Describes how a loop tat is part of a meta-loop, behaves in the context of the meta-loop.
// More specifically, how does the number of iterations and the data cells it visits change over
// time?
class LoopBehavior {
    const MetaLoopAnalysis* _metaLoopAnalysis;
    // The index of the loop in the MetaLoopAnalysis
    int _sequenceIndex;

    std::shared_ptr<LoopAnalysis> _loopAnalysis;

    // How much the mininum (left) and maximum (right) DP value changes on subsequent executions of
    // the loop;
    int _minDpDelta;
    int _maxDpDelta;

    int _iterationDelta;

public:
    LoopBehavior(const MetaLoopAnalysis* metaLoopAnalysis, int sequenceIndex,
                 std::shared_ptr<LoopAnalysis> loopAnalysis,
                 int minDpDelta, int maxDpDelta, int iterationDelta)
    : _metaLoopAnalysis(metaLoopAnalysis)
    , _sequenceIndex(sequenceIndex)
    , _loopAnalysis(loopAnalysis)
    , _minDpDelta(minDpDelta)
    , _maxDpDelta(maxDpDelta)
    , _iterationDelta(iterationDelta) {}

    std::shared_ptr<LoopAnalysis> loopAnalysis() const { return _loopAnalysis; }

    int sequenceIndex() const { return _sequenceIndex; }
    int minDpDelta() const { return _minDpDelta; }
    int maxDpDelta() const { return _maxDpDelta; }
    int iterationDelta() const { return _iterationDelta; }

    const LoopBehavior* prevLoop() const;
    const LoopBehavior* nextLoop() const;

    LoopType loopType() const;
    bool isSweepLoop() const {
        LoopType tp = loopType();
        return tp == LoopType::ANCHORED_SWEEP || tp == LoopType::DOUBLE_SWEEP;
    }
};

// Simple pool that lazily expands when needed. All elements should be returned at once (via reset)
template <typename T>
class SimplePool {
    std::vector<std::shared_ptr<T>> _pool;
    int _numPopped;
public:
    std::shared_ptr<T> pop() {
        if (_numPopped == _pool.size()) {
            _pool.push_back(std::make_shared<T>());
        }
        return _pool[_numPopped++];
    }
    int numPopped() const { return _numPopped; }
    void reset() { _numPopped = 0; }
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

    mutable SimplePool<SequenceAnalysis> _sequenceAnalysisPool;
    mutable SimplePool<LoopAnalysis> _loopAnalysisPool;

    // The analysis of every run block in the meta-loop (size = _metaLoopPeriod)
    std::vector<std::shared_ptr<SequenceAnalysis>> _seqAnalysis;

    // The sequence analysis for unrolled loops. It can be generated on demand, for fixed-sized
    // loops. It is indexed by sequence index.
    mutable std::map<int, std::shared_ptr<SequenceAnalysis>> _unrolledLoopSeqAnalysis;

    // The properties of the loops in the meta-loop. Access by loop index.
    std::vector<MetaLoopData> _loopData;
    std::vector<LoopBehavior> _loopBehaviors;

    // Maps sequence indices to loop indices.
    std::map<int, int> _loopIndexLookup;

    bool _isPeriodic;

    void analyzeRunBlocks(const ExecutionState &executionState);

    void initLoopData(const RunSummary &runSummary, int loopSize);
    bool checkLoopSize(const RunSummary &runSummary, int loopSize);
    bool determineLoopSize(const ExecutionState &executionState);

    // The number of loops in the meta-run block (the meta-loop may contain a multiple of this).
    // It is valid before analysis is complete.
    int numLoops() const { return _loopAnalysisPool.numPopped(); }

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

    const std::vector<std::shared_ptr<SequenceAnalysis>> sequenceAnalysisResults() const {
        return _seqAnalysis;
    }
    const std::shared_ptr<SequenceAnalysis> sequenceAnalysis(int sequenceIndex) const {
        return _seqAnalysis[sequenceIndex % _metaLoopPeriod];
    }

    // Returns an analysis of a fixed-size loop as if it was a plain sequence.
    std::shared_ptr<SequenceAnalysis>
        unrolledLoopSequenceAnalysis(const ExecutionState &executionState,
                                     int sequenceIndex) const;

    const std::vector<LoopBehavior>& loopBehaviors() const { return _loopBehaviors; }

    // Returns how much DP changed after executing the specified run-block. The index is with
    // respect to the run summary.
    int dpDeltaOfRunBlock(const RunSummary& runSummary, int runBlockIndex) const;

    int isLoop(int sequenceIndex) const {
        return _seqAnalysis[sequenceIndex % _metaLoopPeriod]->isLoop();
    }

    int loopIndexForSequence(int sequenceIndex) const { return _loopIndexLookup.at(sequenceIndex); }
    int sequenceIndexForLoop(int loopIndex) const { return _loopData[loopIndex].sequenceIndex; }

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
};
