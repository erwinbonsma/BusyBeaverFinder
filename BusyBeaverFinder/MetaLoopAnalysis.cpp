//
//  MetaLoopAnalysis.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 15/10/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "MetaLoopAnalysis.h"

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

            int loopExit = runSummary.getRunBlockLength(idx) % rb->getLoopPeriod();
            int numIter = runSummary.getRunBlockLength(idx) / rb->getLoopPeriod();

            switch (i) {
                case 0:
                    _seqProps.emplace_back(loopExit);
                    _seqProps.back().numIterations = numIter;
                    break;
                case 1: {
                    auto &props = _seqProps[j];
                    props.iterationDelta += numIter - props.numIterations;
                    if (props.iterationDelta < 0) {
                        // The number of iterations should not decrease
                        return false;
                    }
                    props.numIterations = numIter;
                    break;
                }
                case 2: {
                    auto &props = _seqProps[j];
                    auto seqAnalysis = _seqAnalysis[j % _metaLoopPeriod];
                    int newDelta = numIter - props.numIterations;
                    if (seqAnalysis->dataPointerDelta() != 0) {
                        if (newDelta != props.iterationDelta) {
                            // The number of iterations for non-stationary loops should be fixed or
                            // increase linearly
                            return false;
                        }
                    } else {
                        if (newDelta < props.iterationDelta) {
                            // The number of iterations should not decrease
                            return false;
                        }
                    }
                    break;
                }
            }
        }
    }

    return true;
}

bool MetaLoopAnalysis::findLoopSize(const ExecutionState &executionState) {
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

    _sequenceAnalysisPool.clear();
    _loopAnalysisPool.clear();
    _seqAnalysis.clear();

    _metaLoopPeriod = metaRunSummary.getLoopPeriod();

    int startIndex = runSummary.getNumRunBlocks() - _metaLoopPeriod;
    for (int i = 0; i < _metaLoopPeriod; ++i) {
        const RunBlock * runBlock = runSummary.runBlockAt(startIndex + i);
        SequenceAnalysis *analysis = nullptr;

        if (runBlock->isLoop()) {
            _loopAnalysisPool.emplace_back();
            LoopAnalysis &loopAnalysis = _loopAnalysisPool.back();
            loopAnalysis.analyzeLoop(&runHistory[runBlock->getStartIndex()],
                                     runBlock->getLoopPeriod());
            analysis = &loopAnalysis;
        } else {
            _sequenceAnalysisPool.emplace_back();
            SequenceAnalysis &sequenceAnalysis = _sequenceAnalysisPool.back();
            sequenceAnalysis.analyzeSequence(&runHistory[runBlock->getStartIndex()],
                                             runSummary.getRunBlockLength(startIndex + i));
            analysis = &sequenceAnalysis;
        }

        _seqAnalysis.push_back(analysis);
    }
}

bool MetaLoopAnalysis::analyzeMetaLoop(const ExecutionState &executionState) {
    // TODO: Make lazy (re-use previous results when applicable)?
    analyzeRunBlocks(executionState);

    if (!findLoopSize(executionState)) {
        return false;
    }

    // TODO: Analyze how data boundaries move each iteration

    return true;
}
