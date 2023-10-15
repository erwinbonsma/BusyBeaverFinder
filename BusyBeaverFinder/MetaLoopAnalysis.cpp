//
//  MetaLoopAnalysis.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 15/10/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "MetaLoopAnalysis.h"

bool MetaLoopAnalysis::establishIterationDeltas(const RunSummary &runSummary, int start, int end) {
    bool populate = _iterationDeltas.size() == 0;

    // Initialize iteration deltas
    int idx1 = start;
    int idx2 = end;
    while (idx1 < end) {
        const RunBlock* rb1 = runSummary.runBlockAt(idx1);
        const RunBlock* rb2 = runSummary.runBlockAt(idx2);

        assert(rb1->getSequenceId() == rb2->getSequenceId());

        if (rb1->isLoop()) {
            int len1 = runSummary.getRunBlockLength(idx1);
            int len2 = runSummary.getRunBlockLength(idx2);

            if (len2 < len1 || (len2 - len1) % rb1->getLoopPeriod() != 0) {
                // The number of iterations should increase and both loops should exit at the
                // same program block
                return false;
            }

            int delta = (len2 - len1) / rb1->getLoopPeriod();
            if (populate) {
                _iterationDeltas.push_back(delta);
            } else {
                if (_iterationDeltas[start - idx1] != delta) {
                    return false;
                }
            }
        } else {
            if (populate) {
                _iterationDeltas.push_back(0);
            }
        }

        ++idx1;
        ++idx2;
    }

    return true;
}

bool MetaLoopAnalysis::findLoopSize(const ExecutionState &executionState) {
    const RunSummary &runSummary = executionState.getRunSummary();
    const MetaRunSummary &metaRunSummary = executionState.getMetaRunSummary();
    int metaLoopPeriod = metaRunSummary.getLoopPeriod();

    _loopSize = metaLoopPeriod;

    while (_loopSize * 3 <= metaRunSummary.getLoopIteration()) {
        _iterationDeltas.clear();

        int startIndex = runSummary.getNumRunBlocks() - _loopSize  * 3;

        // Populate iteration deltas
        if (!establishIterationDeltas(runSummary, startIndex, startIndex + _loopSize)) {
            return false;
        }

        // Verify constant growth
        startIndex += _loopSize;
        if (!establishIterationDeltas(runSummary, startIndex, startIndex + _loopSize)) {
            continue;
        }

        return true;
    }

    return false;
}

void MetaLoopAnalysis::analyzeRunBlocks(const ExecutionState &executionState) {
    const RunHistory &runHistory = executionState.getRunHistory();
    const RunSummary &runSummary = executionState.getRunSummary();

    _sequenceAnalysisPool.clear();
    _loopAnalysisPool.clear();
    _sequenceAnalysis.clear();

    int startIndex = runSummary.getNumRunBlocks() - _loopSize;
    for (int i = 0; i < _loopSize; ++i) {
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

        _sequenceAnalysis.push_back(analysis);
    }

}

bool MetaLoopAnalysis::analyzeMetaLoop(const ExecutionState &executionState) {
    if (!findLoopSize(executionState)) {
        return false;
    }

    analyzeRunBlocks(executionState);

    // TODO: Analyze how data boundaries move each iteration

    return true;
}

