//
//  ProgramBlock.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 25/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "ProgramBlock.h"

ProgramBlock::ProgramBlock() {
}

void ProgramBlock::init(int startIndex) {
    _startIndex = startIndex;
    _isFinalized = false;
}

void ProgramBlock::finalize(bool isDelta, int amount, int numSteps,
                            ProgramBlock* zeroBlock, ProgramBlock* nonZeroBlock) {
    assert(!_isFinalized);
    _isFinalized = true;

    _isDelta = isDelta;
    _instructionAmount = amount;
    _numSteps = numSteps;
    _zeroBlock = zeroBlock;
    _nonZeroBlock = nonZeroBlock;
}
