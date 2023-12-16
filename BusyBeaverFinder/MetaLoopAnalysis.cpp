//
//  MetaLoopAnalysis.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 15/10/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "MetaLoopAnalysis.h"

// The maximum number of meta-run loop iterations to unroll to construct a meta-loop that meets
// the criteria.
constexpr int MAX_ITERATIONS_TO_UNROLL = 2;

// The number of meta-loop iterations that are analyzed.
// Note: It should not be changed as its inherent to the analysis; using this constant avoids a
// magic numbers.
constexpr int NUM_ITERATIONS_TO_ANALYZE = 3;

LoopType LoopBehavior::loopType() const {
    if (_minDpDelta == 0 && _maxDpDelta == 0) {
        return LoopType::STATIONARY;
    }
    if (_minDpDelta == _maxDpDelta) {
        return LoopType::GLIDER;
    }
    if (_minDpDelta == 0 || _maxDpDelta == 0) {
        return LoopType::ANCHORED_SWEEP;
    }
    return LoopType::DOUBLE_SWEEP;
}

void MetaLoopAnalysis::initLoopData(const RunSummary &runSummary, int loopSize) {
    _loopData.clear();
    _loopIndexLookup.clear();

    _isPeriodic = true;  // Initial assumption.
    _firstRunBlockIndex = runSummary.getNumRunBlocks() - loopSize * NUM_ITERATIONS_TO_ANALYZE;
    int rbIndex = _firstRunBlockIndex;

    // Initialize properties
    for (int i = 0; i < loopSize; ++i) {
        const RunBlock* rb = runSummary.runBlockAt(rbIndex);

        if (rb->isLoop()) {
            int blockLen = runSummary.getRunBlockLength(rbIndex);
            int loopRemainder = blockLen % rb->getLoopPeriod();
            int numIter = blockLen / rb->getLoopPeriod();

            _loopIndexLookup[i] = static_cast<int>(_loopData.size());
            _loopData.emplace_back(_loopData.size(), i, numIter, loopRemainder);
        }

        rbIndex += 1;
    }
}

bool MetaLoopAnalysis::checkLoopSize(const RunSummary &runSummary, int loopSize) {
    initLoopData(runSummary, loopSize);

    // Analyze loop iteration deltas. Loop twice. First to determine the delta, and next to check
    // if/how this delta changes.
    for (int i = 1; i <= 2; ++i) {
        int loopStartIndex = _firstRunBlockIndex + loopSize * i;
        for (auto &data : _loopData) {
            int rbIndex = loopStartIndex + data.runBlockIndex;
            const RunBlock* rb = runSummary.runBlockAt(rbIndex);

            assert(rb->isLoop());

            auto &dataPrev = _loopData[(data.loopIndex - _numLoops + _loopData.size())
                                       % _loopData.size()];
            int blockLen = runSummary.getRunBlockLength(rbIndex);
            int numIter = blockLen / rb->getLoopPeriod();
            int delta = numIter - dataPrev.lastNumIterations;
            if (delta < 0) {
                // The number of iterations should not decrease
                return false;
            }

            _isPeriodic &= (delta == 0);

            if (i == 2) {
                if (delta < data.lastIterationDelta) {
                    // The number of iterations should not decrease
                    return false;
                }
                if (delta > data.lastIterationDelta) {
                    auto &seqAnalysis = _seqAnalysis[data.runBlockIndex % _metaLoopPeriod];
                    if (seqAnalysis->dataPointerDelta() != 0) {
                        // The number of iterations for non-stationary loops should be fixed or
                        // increase linearly
                        return false;
                    } else {
                        data.isLinear = false;
                    }
                }
            }

            data.lastIterationDelta = delta;
            data.lastNumIterations = numIter;
        }
    }

    return true;
}

bool MetaLoopAnalysis::determineLoopSize(const ExecutionState &executionState) {
    const RunSummary &runSummary = executionState.getRunSummary();
    const MetaRunSummary &metaRunSummary = executionState.getMetaRunSummary();
    int ln = metaRunSummary.getLoopIteration();

    int max_i = std::min(MAX_ITERATIONS_TO_UNROLL, ln / NUM_ITERATIONS_TO_ANALYZE);
    int loopSize = _metaLoopPeriod;
    for (int i = 0; i < max_i; ++i, loopSize += _metaLoopPeriod) {
        if (checkLoopSize(runSummary, loopSize)) {
            _loopSize = loopSize;
            return true;
        }
    }

    return false;
}

void MetaLoopAnalysis::analyzeRunBlocks(const ExecutionState &executionState) {
    const RunHistory &runHistory = executionState.getRunHistory();
    const RunSummary &runSummary = executionState.getRunSummary();
    const MetaRunSummary &metaRunSummary = executionState.getMetaRunSummary();

    _seqAnalysis.clear();
    _numSequences = 0;
    _numLoops = 0;

    _metaLoopPeriod = metaRunSummary.getLoopPeriod();

    int startIndex = runSummary.getNumRunBlocks() - _metaLoopPeriod;
    for (int i = 0; i < _metaLoopPeriod; ++i) {
        const RunBlock *rb = runSummary.runBlockAt(startIndex + i);
        std::shared_ptr<SequenceAnalysis> analysis = nullptr;

        if (rb->isLoop()) {
            while (_numLoops >= _loopAnalysisPool.size()) {
                _loopAnalysisPool.push_back(std::make_shared<LoopAnalysis>());
            }
            auto loopAnalysis = _loopAnalysisPool[_numLoops++];
            loopAnalysis->analyzeLoop(&runHistory[rb->getStartIndex()], rb->getLoopPeriod());
            analysis = loopAnalysis;
        } else {
            while (_numSequences >= _sequenceAnalysisPool.size()) {
                _sequenceAnalysisPool.push_back(std::make_shared<SequenceAnalysis>());
            }
            auto sequenceAnalysis = _sequenceAnalysisPool[_numSequences++];
            sequenceAnalysis->analyzeSequence(&runHistory[rb->getStartIndex()],
                                              runSummary.getRunBlockLength(startIndex + i));
            analysis = sequenceAnalysis;
        }

        _seqAnalysis.push_back(analysis);
    }
}

void MetaLoopAnalysis::determineDpDeltas(const RunSummary &runSummary) {
    int dp = 0;

    int end = runSummary.getNumRunBlocks();
    int start = end - _loopSize * 2;
    int rbIndex = start;

    for (int i = 0; i < 2; ++i) {
        auto loopDataIt = _loopData.begin();
        for (int j = 0; j < _loopSize; ++j, ++rbIndex) {
            const RunBlock *rb = runSummary.runBlockAt(rbIndex);
            auto sa = _seqAnalysis[j % _metaLoopPeriod];

            if (!rb->isLoop()) {
                dp += sa->dataPointerDelta();
                continue;
            }
            auto &data = *loopDataIt++;

//            std::cout << "dp[" << j << "] = " << dp << std::endl;

            if (i == 1) {
                auto &dataPrev = _loopData[(data.loopIndex - _numLoops + _loopData.size())
                                           % _loopData.size()];
                data.dataPointerDelta = dp - dataPrev.dataPointerPos;
            }
            data.dataPointerPos = dp;

            int numIter = runSummary.getRunBlockLength(rbIndex) / rb->getLoopPeriod();
            dp += numIter * sa->dataPointerDelta();
            if (data.loopRemainder > 0) {
                dp += sa->effectiveResultAt(data.loopRemainder - 1).dpOffset();
            }
        }
    }
}

void MetaLoopAnalysis::initLoopBehaviors() {
    _loopBehaviors.clear();

    // For meta-loops where loopSize > metaLoopPeriod accumulate the deltas to get the loop
    // behavior for each loop compared to its execution in the previous meta-loop iteration.
    //
    // Note: there is currently duplication in data (and calculation) as loopBehavior[i] equals
    // loopBehavior[j] when i % loopPeriod == j % loopPeriod. That's okay for now. This is fairly
    // rare, and when analysis is extended, the equality may not hold anymore.

    for (auto &data : _loopData) {
        int index = data.runBlockIndex % _metaLoopPeriod;
        auto sa = _seqAnalysis[index];

        assert(sa->isLoop());
        int dpDeltaStart = 0;
        int iterDelta = 0;
        bool isLinear = true;

        for (int j = data.loopIndex % _numLoops; j < _loopData.size(); j += _numLoops) {
            auto &data = _loopData[j];
            dpDeltaStart += data.dataPointerDelta;
            iterDelta += data.lastIterationDelta;
            isLinear &= data.isLinear;
        }

        int dpDeltaEnd = dpDeltaStart + iterDelta * sa->dataPointerDelta();

        _loopBehaviors.emplace_back(std::dynamic_pointer_cast<LoopAnalysis>(sa),
                                    _loopBehaviors.size(),
                                    std::min(dpDeltaStart, dpDeltaEnd),
                                    std::max(dpDeltaStart, dpDeltaEnd),
                                    isLinear ? iterDelta : -1);
    }
}

void MetaLoopAnalysis::reset() {
    _numRunBlocks = 0;
    _numMetaRunBlocks = 0;

    _loopData.clear();
    _loopBehaviors.clear();
    _seqAnalysis.clear();
}

bool MetaLoopAnalysis::isAnalysisStillValid(const ExecutionState &executionState) {
    if (_numRunBlocks == 0) {
        return false;  // There is no analysis yet
    }

    if (_numMetaRunBlocks != executionState.getMetaRunSummary().getNumRunBlocks()) {
        return false;  // The meta-loop changed
    }

    // Check if the previous analysis still applies
    assert(_numRunBlocks < executionState.getRunSummary().getNumRunBlocks());

    int rbIndex = _numRunBlocks;
    auto &runSummary = executionState.getRunSummary();

    while (rbIndex < runSummary.getNumRunBlocks()) {
        int relIndex = (rbIndex - _firstRunBlockIndex) % _loopSize;
        auto sa = _seqAnalysis[relIndex % _metaLoopPeriod];

        if (sa->isLoop()) {
            const RunBlock *rb = runSummary.runBlockAt(rbIndex);
            int loopIndex = _loopIndexLookup[relIndex];
            auto &data = _loopData[loopIndex];
            auto &behavior = _loopBehaviors[loopIndex];

            int blockLen = runSummary.getRunBlockLength(rbIndex);
            int numIter = blockLen / rb->getLoopPeriod();
            int delta = numIter - data.lastNumIterations;

            if (behavior.iterationDelta() >= 0) {
                // Delta remain the same for constant-sized and linearly growing loops
                if (delta != behavior.iterationDelta()) {
                    reset();
                    return false;
                }
            } else {
                // Delta should increase for non-linearly growing loops
                if (delta < behavior.iterationDelta()) {
                    reset();
                    return false;
                }
            }

            data.lastNumIterations = numIter;
            data.lastIterationDelta = delta;
        }
        rbIndex += 1;
    }

    _numRunBlocks = executionState.getRunSummary().getNumRunBlocks();

    return true;
}

bool MetaLoopAnalysis::analyzeMetaLoop(const ExecutionState &executionState) {
    assert(executionState.getMetaRunSummary().isInsideLoop());

    analyzeRunBlocks(executionState);

    if (!determineLoopSize(executionState)) {
        return false;
    }

    determineDpDeltas(executionState.getRunSummary());
    initLoopBehaviors();

    // Mark results valid
    _numRunBlocks = executionState.getRunSummary().getNumRunBlocks();
    _numMetaRunBlocks = executionState.getMetaRunSummary().getNumRunBlocks();

    return true;
}
