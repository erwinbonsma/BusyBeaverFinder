//
//  MetaLoopAnalysis.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 15/10/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "MetaLoopAnalysis.h"

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

bool MetaLoopAnalysis::checkLoopSize(const RunSummary &runSummary, int loopSize) {
    _seqProps.clear();

    int end = runSummary.getNumRunBlocks();
    int start = end - loopSize * 3;
    int idx = start;

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < loopSize; ++j, ++idx) {
            const RunBlock* rb = runSummary.runBlockAt(idx);

            if (!rb->isLoop()) {
                if (i == 0) {
                    _seqProps.emplace_back();
                }
                continue;
            }

            int loopRemainder = runSummary.getRunBlockLength(idx) % rb->getLoopPeriod();
            int numIter = runSummary.getRunBlockLength(idx) / rb->getLoopPeriod();

            if (i == 0) {
                _seqProps.emplace_back(loopRemainder);
                _seqProps[j].numIterations = numIter;
                continue;
            }

            auto &propsPrev = _seqProps[(j + loopSize - _metaLoopPeriod) % loopSize];
            auto &props = _seqProps[j];
            int delta = numIter - propsPrev.numIterations;

            if (i == 1) {
                if (delta < 0) {
                    // The number of iterations should not decrease
                    return false;
                }
                props.iterationDelta = delta;
                props.numIterations = numIter;
                continue;
            }

            auto &seqAnalysis = _seqAnalysis[j % _metaLoopPeriod];
            if (seqAnalysis->dataPointerDelta() != 0) {
                if (delta != props.iterationDelta) {
                    // The number of iterations for non-stationary loops should be fixed or
                    // increase linearly
                    return false;
                }
            } else {
                if (delta < props.iterationDelta) {
                    // The number of iterations should not decrease
                    return false;
                }
                if (delta != props.iterationDelta) {
                    // Signal that increase is non-linear
                    props.iterationDelta = -1;
                }
            }
            props.numIterations = numIter;
        }
    }

    return true;
}

bool MetaLoopAnalysis::determineLoopSize(const ExecutionState &executionState) {
    const RunSummary &runSummary = executionState.getRunSummary();
    const MetaRunSummary &metaRunSummary = executionState.getMetaRunSummary();
    int ln = metaRunSummary.getLoopIteration();

    for (int loopSize = _metaLoopPeriod;
         loopSize * 3 <= ln * _metaLoopPeriod;
         loopSize += _metaLoopPeriod) {
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
    int numSequences = 0;
    int numLoops = 0;

    _metaLoopPeriod = metaRunSummary.getLoopPeriod();

    int startIndex = runSummary.getNumRunBlocks() - _metaLoopPeriod;
    for (int i = 0; i < _metaLoopPeriod; ++i) {
        const RunBlock *rb = runSummary.runBlockAt(startIndex + i);
        std::shared_ptr<SequenceAnalysis> analysis = nullptr;

        if (rb->isLoop()) {
            while (numLoops >= _loopAnalysisPool.size()) {
                _loopAnalysisPool.push_back(std::make_shared<LoopAnalysis>());
            }
            auto loopAnalysis = _loopAnalysisPool[numLoops++];
            loopAnalysis->analyzeLoop(&runHistory[rb->getStartIndex()], rb->getLoopPeriod());
            analysis = loopAnalysis;
        } else {
            while (numSequences >= _sequenceAnalysisPool.size()) {
                _sequenceAnalysisPool.push_back(std::make_shared<SequenceAnalysis>());
            }
            auto sequenceAnalysis = _sequenceAnalysisPool[numSequences++];
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
    int idx = start;

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < _loopSize; ++j, ++idx) {
            auto &props = _seqProps[j];

//            std::cout << "dp[" << j << "] = " << dp << std::endl;

            if (i == 1) {
                auto &propsPrev = _seqProps[(j + _loopSize - _metaLoopPeriod) % _loopSize];
                props.dataPointerDelta = dp - propsPrev.dataPointerPos;
            }
            props.dataPointerPos = dp;

            const RunBlock *rb = runSummary.runBlockAt(idx);
            auto sa = _seqAnalysis[j % _metaLoopPeriod];
            int delta;

            if (rb->isLoop()) {
                int numIter = runSummary.getRunBlockLength(idx) / rb->getLoopPeriod();
                delta = numIter * sa->dataPointerDelta();
                if (props.loopRemainder > 0) {
                    delta += sa->effectiveResultAt(props.loopRemainder - 1).dpOffset();
                }
            } else {
                delta = sa->dataPointerDelta();
            }

            dp += delta;
        }
    }
}

void MetaLoopAnalysis::initLoopBehaviors() {
    _loopBehaviors.clear();

    for (int i = 0; i < _loopSize; ++i) {
        int index = i % _metaLoopPeriod;
        auto &sa = _seqAnalysis[index];
        if (sa->isLoop()) {
            int dpDeltaStart = 0;
            int iterDelta = 0;

            for (int j = 0; j < _loopSize; j += _metaLoopPeriod) {
                auto &sp = _seqProps[index + j];
                dpDeltaStart += sp.dataPointerDelta;
                iterDelta += sp.iterationDelta;
            }

            int dpDeltaEnd = dpDeltaStart + iterDelta * sa->dataPointerDelta();

            _loopBehaviors.emplace_back(std::dynamic_pointer_cast<LoopAnalysis>(sa),
                                        std::min(dpDeltaStart, dpDeltaEnd),
                                        std::max(dpDeltaStart, dpDeltaEnd),
                                        iterDelta);
        }
    }
}

bool MetaLoopAnalysis::analyzeMetaLoop(const ExecutionState &executionState) {
    // TODO: Make lazy (re-use previous results when applicable)?
    analyzeRunBlocks(executionState);

    if (!determineLoopSize(executionState)) {
        return false;
    }

    determineDpDeltas(executionState.getRunSummary());
    initLoopBehaviors();

    return true;
}
