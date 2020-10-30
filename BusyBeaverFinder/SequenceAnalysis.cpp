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

void SequenceAnalysis::analyzeSequence() {
    _dpDelta = 0;
    _dataDeltas.clear();
    _effectiveResult.clear();

    // Determine the intermediate results and final results of a single loop iteration
    _minDp =
        (sequenceSize() == 0 || _programBlocks[0]->isDelta())
        ? 0 : _programBlocks[0]->getInstructionAmount();
    _maxDp = _minDp;

    for (const ProgramBlock* programBlock : _programBlocks) {
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
    os << "delta DP = " << sa.dataPointerDelta();

    for (int i = 0; i < sa.numDataDeltas(); i++) {
        const DataDelta& dd = sa.dataDeltaAt(i);

        os << ", ";
        os << "[" << dd.dpOffset() << "]";
        if (dd.delta() > 0) {
            os << "+";
        }
        os << dd.delta();
    }

    return os;
}
