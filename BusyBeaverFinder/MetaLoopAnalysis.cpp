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

// Replace by std::optional monadic operations after switch to C++23
std::string optionalIntToString(std::optional<int> value) {
    return value ? std::to_string(value.value()) : "N/A";
}

std::ostream &operator<<(std::ostream &os, const LoopBehavior &behavior) {
    os << "minDpDelta = " << optionalIntToString(behavior.minDpDelta())
    << ", maxDpDelta = " << optionalIntToString(behavior.maxDpDelta())
    << ", iterationDelta = " << behavior.iterationDelta();

    return os;
}


void MetaLoopAnalysis::initLoopData(const RunSummary &runSummary, int loopSize) {
    _loopData.clear();
    _loopIndexLookup.clear();

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

MetaLoopType MetaLoopAnalysis::checkLoopSize(const RunSummary &runSummary, int loopSize) {
    initLoopData(runSummary, loopSize);

    MetaLoopType loopType = MetaLoopType::PERIODIC;  // Initial assumption

    // Analyze loop iteration deltas. Loop twice. First to determine the delta, and next to check
    // if/how this delta changes.
    for (int i = 1; i <= 2; ++i) {
        int loopStartIndex = _firstRunBlockIndex + loopSize * i;
        for (auto &data : _loopData) {
            int rbIndex = loopStartIndex + data.sequenceIndex;
            const RunBlock* rb = runSummary.runBlockAt(rbIndex);

            assert(rb->isLoop());

            bool isStationary = (_seqAnalysis[data.sequenceIndex
                                              % _metaLoopPeriod]->dataPointerDelta() == 0);

            int blockLen = runSummary.getRunBlockLength(rbIndex);
            int numIter = blockLen / rb->getLoopPeriod();
            int delta = numIter - data.lastNumIterations;

            if (delta < 0) {
                data.iterationDeltaType = LoopIterationDelta::IRREGULAR;

                if (isStationary) {
                    // The number of iterations for stationary loops cannot decrease.
                    return MetaLoopType::UNSUPPORTED;
                }

                // A (temporary) decrease is possible for irregular sweep loops
                loopType = MetaLoopType::IRREGULAR;
            } else if (delta > 0) {
                // Assume increase is linear
                data.iterationDeltaType = std::max(data.iterationDeltaType,
                                                   LoopIterationDelta::LINEAR_INCREASE);

                // Meta-loop is not periodic. Assume it is regular instead.
                loopType = std::max(loopType, MetaLoopType::REGULAR);
            }

            if (i == 2) {
                if (data.lastIterationDelta < 0 && delta <= data.lastIterationDelta) {
                    // For irregular sweep loops, only analysis of binary-counting type behaviors
                    // is supported. In this case, there should never be two subsequent decreases
                    // of (sweep) loop iteration count.
                    return MetaLoopType::UNSUPPORTED;
                }

                if (delta == data.lastIterationDelta) {
                    // Nothing needs doing. iterationDeltaType is already correct
                    assert((delta == 0
                            && data.iterationDeltaType == LoopIterationDelta::CONSTANT) ||
                           (delta > 0
                            && data.iterationDeltaType == LoopIterationDelta::LINEAR_INCREASE));
                }
                if (delta < data.lastIterationDelta) {
                    data.iterationDeltaType = LoopIterationDelta::IRREGULAR;

                    if (isStationary) {
                        // The change in number of iterations for stationary loops cannot decrease.
                        return MetaLoopType::UNSUPPORTED;
                    }
                    loopType = MetaLoopType::IRREGULAR;
                }
                if (delta > data.lastIterationDelta) {
                    data.iterationDeltaType = LoopIterationDelta::NONLINEAR_INCREASE;

                    if (!isStationary) {
                        // The number of iterations for non-stationary loops cannot increase non-
                        // linearly
                        return MetaLoopType::UNSUPPORTED;
                    }

                    // Meta-loop is not periodic. Assume it is regular instead.
                    loopType = std::max(loopType, MetaLoopType::REGULAR);
                }
            }

            data.lastIterationDelta = delta;
            data.lastNumIterations = numIter;
        }
    }

    return loopType;
}

bool MetaLoopAnalysis::determineLoopSize(const ExecutionState &executionState) {
    const RunSummary &runSummary = executionState.getRunSummary();
    const MetaRunSummary &metaRunSummary = executionState.getMetaRunSummary();
    int ln = metaRunSummary.getLoopIteration();

    MetaLoopType bestLoopType = MetaLoopType::UNSUPPORTED;
    int bestLoopSize = 0;

    int max_i = std::min(MAX_ITERATIONS_TO_UNROLL, ln / NUM_ITERATIONS_TO_ANALYZE);
    int loopSize = _metaLoopPeriod;
    for (int i = 0; i < max_i; ++i, loopSize += _metaLoopPeriod) {
        auto loopType = checkLoopSize(runSummary, loopSize);

        if (loopType < bestLoopType) {
            bestLoopType = loopType;
            bestLoopSize= loopSize;
        }
    }

    if (bestLoopType == MetaLoopType::UNSUPPORTED) {
        return false;
    }

    if (bestLoopSize != loopSize) {
        // Restore state to match the best found loop size
        auto result = checkLoopSize(runSummary, bestLoopSize);
        assert(result == bestLoopType);
    }

    _metaLoopType = bestLoopType;
    _analysisLoopSize = bestLoopSize;
    return true;
}

void MetaLoopAnalysis::analyzeRunBlocks(const ExecutionState &executionState) {
    const RunHistory &runHistory = executionState.getRunHistory();
    const RunSummary &runSummary = executionState.getRunSummary();
    const MetaRunSummary &metaRunSummary = executionState.getMetaRunSummary();

    _seqAnalysis.clear();
    _sequenceAnalysisPool.reset();
    _loopAnalysisPool.reset();

    _metaLoopPeriod = metaRunSummary.getLoopPeriod();

    int startIndex = runSummary.getNumRunBlocks() - _metaLoopPeriod;
    for (int i = 0; i < _metaLoopPeriod; ++i) {
        const RunBlock *rb = runSummary.runBlockAt(startIndex + i);
        std::shared_ptr<SequenceAnalysis> analysis = nullptr;

        if (rb->isLoop()) {
            auto loopAnalysis = _loopAnalysisPool.pop();
            loopAnalysis->analyzeLoop(&runHistory[rb->getStartIndex()], rb->getLoopPeriod());
            analysis = loopAnalysis;
        } else {
            auto sequenceAnalysis = _sequenceAnalysisPool.pop();
            sequenceAnalysis->analyzeSequence(&runHistory[rb->getStartIndex()],
                                              runSummary.getRunBlockLength(startIndex + i));
            analysis = sequenceAnalysis;
        }

        _seqAnalysis.push_back(analysis);
    }
}

int MetaLoopAnalysis::dpDeltaOfRunBlock(const RunSummary& runSummary, int rbIndex) const {
    assert(rbIndex >= _firstRunBlockIndex);

    const RunBlock *rb = runSummary.runBlockAt(rbIndex);
    auto sa = _seqAnalysis[(rbIndex - _firstRunBlockIndex) % _metaLoopPeriod];

    int delta = sa->dataPointerDelta();

    if (rb->isLoop()) {
        int numIter = runSummary.getRunBlockLength(rbIndex) / rb->getLoopPeriod();
        delta *= numIter;

        int blockLen = runSummary.getRunBlockLength(rbIndex);
        int loopRemainder = blockLen % rb->getLoopPeriod();
        if (loopRemainder > 0) {
            delta += sa->effectiveResultAt(loopRemainder - 1).dpOffset();
        }
    }

    return delta;
}

// Determine how the DP position at the end of each loop changes wrt to the its DP position at
// the end of the loop in the previous iteration of the _analyzed_ meta-loop.
void MetaLoopAnalysis::determineDpDeltas(const RunSummary &runSummary) {
    int dp = 0;

    int end = runSummary.getNumRunBlocks();
    int start = end - _analysisLoopSize * 3;
    int rbIndex = start;

    // Loop three times:
    // First to determine the DP positions at the start of each loop.
    // Second to determine how it changed with respect to the previous loop execution (in the
    //   context of the analyze meta-loop).
    // Third to check if this delta is constant.
    for (int i = 0; i < 3; ++i) {
        auto loopDataIt = _loopData.begin();
        for (int j = 0; j < _analysisLoopSize; ++j, ++rbIndex) {
            const RunBlock *rb = runSummary.runBlockAt(rbIndex);
            if (rb->isLoop()) {
                auto &data = *loopDataIt++;

                if (i == 1) {
                    data.dataPointerDelta = dp - data.lastDataPointerStartPos;
                } else if (i == 2) {
                    if (data.dataPointerDelta != (dp - data.lastDataPointerStartPos)) {
                        // Clear delta as it is not constant
                        data.dataPointerDelta = {};

                        // TODO: Find out why this is needed. I.e. when the delta is not regular
                        // but the meta-loop type still was.
                        _metaLoopType = MetaLoopType::IRREGULAR;
                    }
                }
                data.lastDataPointerStartPos = dp;
            }
            dp += dpDeltaOfRunBlock(runSummary, rbIndex);
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

    size_t loopDataSize = _loopData.size();
    for (int i = 0; i < loopDataSize; ++i) {
        auto &data = _loopData[i];
        auto dpDeltaStart = data.dataPointerDelta;

        auto sa = _seqAnalysis[data.sequenceIndex % _metaLoopPeriod];
        assert(sa->isLoop());

        LoopIterationDelta loopDeltaType = data.iterationDeltaType;
        bool isLinear = (loopDeltaType == LoopIterationDelta::LINEAR_INCREASE ||
                         loopDeltaType == LoopIterationDelta::CONSTANT);

        auto &dataNext = _loopData[(i + 1) % loopDataSize];
        auto dpDeltaEnd = dataNext.dataPointerDelta;

        assert(sa->dataPointerDelta() != 0 || dpDeltaStart == dpDeltaEnd);
        bool movesLeft = sa->dataPointerDelta() < 0;

        _loopBehaviors.emplace_back(this, data.sequenceIndex,
                                    std::dynamic_pointer_cast<LoopAnalysis>(sa),
                                    movesLeft ? dpDeltaEnd : dpDeltaStart,
                                    movesLeft ? dpDeltaStart : dpDeltaEnd,
                                    isLinear ? data.lastIterationDelta : -1);
    }
}


std::shared_ptr<SequenceAnalysis>
    MetaLoopAnalysis::unrolledLoopSequenceAnalysis(const ExecutionState &executionState,
                                                   int sequenceIndex) const
{
    assert(_seqAnalysis[sequenceIndex]->isLoop());
    int loopIndex = loopIndexForSequence(sequenceIndex);
    assert(_loopBehaviors[loopIndex].iterationDelta() == 0);

    auto result = _unrolledLoopSeqAnalysis.find(sequenceIndex);
    if (result != _unrolledLoopSeqAnalysis.end()) {
        return result->second;
    }

    const RunHistory &runHistory = executionState.getRunHistory();
    const RunSummary &runSummary = executionState.getRunSummary();

    int rbIndex = _firstRunBlockIndex + sequenceIndex;
    int pbStart = runSummary.runBlockAt(rbIndex)->getStartIndex();
    auto sa = _sequenceAnalysisPool.pop();
    sa->analyzeSequence(&runHistory[pbStart], runSummary.getRunBlockLength(rbIndex));

    _unrolledLoopSeqAnalysis[sequenceIndex] = sa;

    return sa;
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
        int relIndex = (rbIndex - _firstRunBlockIndex) % _analysisLoopSize;
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

    // Mark results valid/initialized
    _numRunBlocks = executionState.getRunSummary().getNumRunBlocks();
    _numMetaRunBlocks = executionState.getMetaRunSummary().getNumRunBlocks();

    return true;
}

void MetaLoopAnalysis::dump() const {
    std::cout << *this << std::endl;
}

std::ostream &operator<<(std::ostream &os, const MetaLoopAnalysis &mla) {
    os << "loopPeriod = " << mla.metaLoopPeriod()
    << ", loopSize = " << mla.loopSize()
    << ", firstRunBlockIndex = " << mla.firstRunBlockIndex()
    << std::endl;

    os << "Sequences:" << std::endl;
    for (int i = 0; i < mla.metaLoopPeriod(); ++i) {
        os << i << ". " << *mla.sequenceAnalysisResults()[i] << std::endl;
    }

    os << "Loop behaviors:" << std::endl;
    for (auto& behavior : mla.loopBehaviors()) {
        os << behavior.sequenceIndex() << ". " << behavior << std::endl;
    }

    return os;
}
