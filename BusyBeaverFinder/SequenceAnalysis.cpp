//
//  SequenceAnalysis.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "SequenceAnalysis.h"

#include <algorithm>
#include <iostream>

#include "InterpretedProgram.h"
#include "ProgramBlock.h"
#include "RunSummary.h"

SequenceAnalysis::SequenceAnalysis() : _effectiveResult(32) {
    _dpDelta = 0;
}

void SequenceAnalysis::addPreCondition(int dpOffset, PreCondition preCondition) {
    auto iter = _preConditions.find(dpOffset);

    if (iter != _preConditions.end()) {
        // Some pre-conditions already exist for this value

        if (preCondition.shouldEqual()) {
            if (iter->second.shouldEqual()) {
                // Pre-condition already exists. Nothing needs doing
                assert(iter->second.value() == preCondition.value());
                return;
            } else {
                // Remove all other (shouldNotEqual) conditions. They are replaced by this stricter
                // one
                _preConditions.erase(iter, _preConditions.upper_bound(dpOffset));
            }
        } else {
            if (iter->second.shouldEqual()) {
                // Another, more strict, pre-condtion already exists. Nothing needs doing
                assert(iter->second.value() != preCondition.value());
                return;
            } else {
                // Check that pre-condition does not yet exist
                while (iter != _preConditions.end() && iter->first == dpOffset) {
                    if (iter->second.value() == preCondition.value()) {
                        return; // Pre-condition already exists. Nothing needs doing
                    }
                    ++iter;
                }
            }
        }
    }

    _preConditions.insert({dpOffset, preCondition});
}

void SequenceAnalysis::analyzeSequence() {
    _dpDelta = 0;
    _dataDeltas.clear();
    _effectiveResult.clear();
    _preConditions.clear();

    // Determine the intermediate results and final results of a single loop iteration
    _minDp =
        (sequenceSize() == 0 || _programBlocks[0]->isDelta())
        ? 0 : _programBlocks[0]->getInstructionAmount();
    _maxDp = _minDp;

    const ProgramBlock* prevProgramBlock = nullptr;
    for (const ProgramBlock* programBlock : _programBlocks) {
        if (prevProgramBlock != nullptr) {
            // Add pre-condition
            addPreCondition(_dpDelta, PreCondition(-_dataDeltas.deltaAt(_dpDelta),
                                                   prevProgramBlock->zeroBlock() == programBlock));
        }
        prevProgramBlock = programBlock;

        int amount = programBlock->getInstructionAmount();
        if (programBlock->isDelta()) {
            int effectiveDelta = _dataDeltas.updateDelta(_dpDelta, amount);

            _effectiveResult.push_back(DataDelta(_dpDelta, effectiveDelta));
        } else {
            _dpDelta += amount;
            _minDp = std::min(_minDp, _dpDelta);
            _maxDp = std::max(_maxDp, _dpDelta);

            _effectiveResult.push_back(DataDelta(_dpDelta, _dataDeltas.deltaAt(_dpDelta)));
        }
    }
}

bool SequenceAnalysis::analyzeSequence(const ProgramBlock* entryBlock, int numBlocks) {
    _programBlocks.clear();
    for (int i = 0; i < numBlocks; ++i) {
        _programBlocks.push_back(entryBlock + i);
    }

    analyzeSequence();

    return true;
}

bool SequenceAnalysis::analyzeSequence(const InterpretedProgram& program,
                                       const RunSummary& runSummary, int startIndex, int length) {
    _programBlocks.clear();
    for (int i = startIndex, end = startIndex + length; i < end; ++i) {
        int pb_index = runSummary.programBlockIndexAt(i);
        _programBlocks.push_back(program.programBlockAt(pb_index));
    }

    analyzeSequence();

    return true;
}

bool SequenceAnalysis::hasPreCondition(int dpOffset, PreCondition preCondition) const {
    auto it = _preConditions.find(dpOffset);

    while (it != _preConditions.end() && it->first == dpOffset) {
        if (it->second == preCondition) {
            return true;
        }
        ++it;
    }

    return false;
}

DataDeltas anyDeltasWorkArray;
bool SequenceAnalysis::anyDataDeltasUpUntil(int index) const {
    anyDeltasWorkArray.clear();

    int dpDelta = 0;
    for (const ProgramBlock* programBlock : _programBlocks) {
        int amount = programBlock->getInstructionAmount();
        if (programBlock->isDelta()) {
            anyDeltasWorkArray.updateDelta(dpDelta, amount);
        } else {
            dpDelta += amount;
        }
    }

    return anyDeltasWorkArray.numDeltas() > 0;
}

std::ostream &operator<<(std::ostream &os, const SequenceAnalysis &sa) {
    os << "[" << sa.typeString() << "]";
    os << " pre-conditions: ";
    int lastDpOffset = INT_MAX;
    for (auto it = sa._preConditions.cbegin(), end = sa._preConditions.cend(); it != end; ++it) {
        int dpOffset = it->first;
        if (dpOffset != lastDpOffset) {
            if (it != sa._preConditions.cbegin()) {
                os << ", ";
            }
            os << "[" << dpOffset << "]";
            os << (it->second.shouldEqual() ? "==" : "!=");
        } else {
            os << "&";
        }
        os << it->second.value();

        lastDpOffset = dpOffset;
    }
    if (lastDpOffset == INT_MAX) {
        os << "none";
    }

    os << ", delta DP = " << sa.dataPointerDelta();

    os << ", deltas: ";
    for (int i = 0; i < sa.numDataDeltas(); i++) {
        const DataDelta& dd = sa.dataDeltaAt(i);

        if (i != 0) {
            os << ", ";
        }
        os << "[" << dd.dpOffset() << "]";
        if (dd.delta() > 0) {
            os << "+";
        }
        os << dd.delta();
    }
    if (sa.numDataDeltas() == 0) {
        os << "none";
    }

    return os;
}
