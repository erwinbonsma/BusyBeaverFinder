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

SequenceAnalysis::SequenceAnalysis() {
    _dpDelta = 0;
    _numDataDeltas = 0;
}

int SequenceAnalysis::deltaAt(int dpOffset) {
    int deltaIndex = 0;

    // Find existing delta record, if any
    while (deltaIndex < _numDataDeltas && _dataDelta[deltaIndex].dpOffset() != dpOffset) {
        deltaIndex++;
    }

    return (deltaIndex == _numDataDeltas) ? 0 : _dataDelta[deltaIndex].delta();
}

int SequenceAnalysis::updateDelta(int dpOffset, int delta) {
    int deltaIndex = 0;

    // Find existing delta record, if any
    while (deltaIndex < _numDataDeltas && _dataDelta[deltaIndex].dpOffset() != dpOffset) {
        deltaIndex++;
    }

    if (deltaIndex == _numDataDeltas) {
        // An existing record was not found,  so create one.
        assert(_numDataDeltas < maxDataDeltasPerSequence);
        _dataDelta[_numDataDeltas++].init(dpOffset);
    }

    if (_dataDelta[deltaIndex].changeDelta(delta)) {
        // This change cancelled out previous changes. Remove the entry to reflect this
        if (deltaIndex != --_numDataDeltas) {
            _dataDelta[deltaIndex] = _dataDelta[_numDataDeltas];
        }

        return 0;
    } else {
        return _dataDelta[deltaIndex].delta();
    }
}

void SequenceAnalysis::analyseSequence() {
    _dpDelta = 0;
    _numDataDeltas = 0;

    int i = 0;

    // Determine the intermediate results and final results of a single loop iteration
    _minDp = _programBlocks[0]->isDelta() ? 0 : _programBlocks[0]->getInstructionAmount();
    _maxDp = _minDp;

    while (i < _numBlocks) {
        ProgramBlock* programBlock = _programBlocks[i];
        if (programBlock->isDelta()) {
            int effectiveDelta = updateDelta(_dpDelta, programBlock->getInstructionAmount());
            _effectiveResult[i].init(_dpDelta);
            _effectiveResult[i].changeDelta(effectiveDelta);
        } else {
            _dpDelta += programBlock->getInstructionAmount();
            _minDp = std::min(_minDp, _dpDelta);
            _maxDp = std::max(_maxDp, _dpDelta);

            _effectiveResult[i].init(_dpDelta);
            _effectiveResult[i].changeDelta(deltaAt(_dpDelta));
        }

        i++;
    }
}

bool SequenceAnalysis::analyseSequence(ProgramBlock* entryBlock, int numBlocks) {
    if (numBlocks > maxSequenceSize) {
        // This sequence is too large to analyse
        return false;
    }

    _numBlocks = numBlocks;
    for (int i = _numBlocks; --i >= 0; ) {
        _programBlocks[i] = entryBlock + i;
    }

    analyseSequence();

    return true;
}

bool SequenceAnalysis::analyseSequence(InterpretedProgram& program, RunSummary& runSummary,
                                       int startIndex, int length) {
    if (length > maxSequenceSize) {
        // This sequence is too large to analyse
        return false;
    }

    _numBlocks = length;
    for (int i = _numBlocks; --i >= 0; ) {
        _programBlocks[i] = program.getEntryBlock() + runSummary.programBlockIndexAt(startIndex+i);
    }

    analyseSequence();

    return true;
}

void SequenceAnalysis::dump() {
    std::cout << "delta DP = " << _dpDelta << std::endl;

    for (int i = 0; i < _numDataDeltas; i++) {
        if (i > 0) {
            std::cout << ", ";
        }
        std::cout << "[" << _dataDelta[i].dpOffset() << "]";
        if (_dataDelta[i].delta() > 0) {
            std::cout << "+";
        }
        std::cout << _dataDelta[i].delta();
    }
    std::cout << std::endl;
}

