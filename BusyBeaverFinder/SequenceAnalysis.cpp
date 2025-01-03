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

bool PreCondition::holdsForValue(int value) const {
    assert(!_invalid);
    return (_shouldEqual == (value == _value));
}

void SequenceAnalysis::addPreCondition(int dpOffset, PreCondition preCondition) {
    auto iter = _preConditions.find(dpOffset);

    if (iter != _preConditions.end()) {
        // Some pre-conditions already exist for this value

        if (preCondition.shouldEqual()) {
            if (iter->second.shouldEqual()) {
                // Pre-condition already exists. Nothing needs doing
                if (iter->second.value() != preCondition.value()) {
                    preCondition.invalidate();
                }
                return;
            } else {
                // Remove all other (shouldNotEqual) conditions. They are replaced by this stricter
                // one
                _preConditions.erase(iter, _preConditions.upper_bound(dpOffset));
            }
        } else {
            if (iter->second.shouldEqual()) {
                // Another, more strict, pre-condtion already exists. Nothing needs doing
                if (iter->second.value() == preCondition.value()) {
                    preCondition.invalidate();
                }
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

void SequenceAnalysis::startAnalysis() {
    _dpDelta = 0;
    _dataDeltas.clear();
    _effectiveResult.clear();
    _preConditions.clear();

    _minDp = std::numeric_limits<int>::max();
    _maxDp = std::numeric_limits<int>::min();

    _programBlocks = nullptr;
    _numProgramBlocks = 0;
}

void SequenceAnalysis::analyzeBlock(const ProgramBlock* pb) {
    if (_prevProgramBlock != nullptr) {
        // Add pre-condition
        addPreCondition(_dpDelta, PreCondition(-_dataDeltas.deltaAt(_dpDelta),
                                               _prevProgramBlock->zeroBlock() == pb));
    }
    _prevProgramBlock = pb;

    int amount = pb->getInstructionAmount();
    if (pb->isDelta()) {
        int effectiveDelta = _dataDeltas.updateDelta(_dpDelta, amount);

        _effectiveResult.emplace_back(_dpDelta, effectiveDelta);
    } else {
        _dpDelta += amount;
        _minDp = std::min(_minDp, _dpDelta);
        _maxDp = std::max(_maxDp, _dpDelta);

        _effectiveResult.emplace_back(_dpDelta, _dataDeltas.deltaAt(_dpDelta));
    }
}

void SequenceAnalysis::analyzeBlocks(RawProgramBlocks programBlocks, int len) {
    _minDp = std::min(_minDp, _dpDelta);
    _maxDp = std::max(_maxDp, _dpDelta);
    _prevProgramBlock = nullptr;

    auto end = programBlocks + len;
    for (auto pb = programBlocks; pb != end; ++pb) {
        analyzeBlock(*pb);
    }

    _numProgramBlocks += len;
}

bool SequenceAnalysis::finishAnalysis() {
    return true;
}

bool SequenceAnalysis::analyzeSequence(RawProgramBlocks programBlocks, int len) {
    startAnalysis();

    _programBlocks = programBlocks;
    analyzeBlocks(programBlocks, len);

    return finishAnalysis();
}

void SequenceAnalysis::analyzeMultiSequenceStart() {
    startAnalysis();
}

void SequenceAnalysis::analyzeMultiSequence(RawProgramBlocks programBlocks, int len, int dpDelta) {
    _dpDelta = dpDelta;

    analyzeBlocks(programBlocks, len);
}

bool SequenceAnalysis::analyzeMultiSequenceEnd(int dpDelta) {
    _dpDelta = dpDelta;

    return finishAnalysis();
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
    int num = 0;
    for (auto dd : sa.dataDeltas()) {
        if (num++ != 0) {
            os << ", ";
        }
        os << "[" << dd.dpOffset() << "]";
        if (dd.delta() > 0) {
            os << "+";
        }
        os << dd.delta();
    }
    if (num == 0) {
        os << "none";
    }

    return os;
}
