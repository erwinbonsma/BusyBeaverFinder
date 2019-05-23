//
//  ProgramBlock.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 25/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "ProgramBlock.h"

#include <iostream>

ProgramBlock::ProgramBlock() {
}

void ProgramBlock::init(int startIndex) {
    _startIndex = startIndex;
    _isFinalized = false;
    _numEntries = 0;
}

void ProgramBlock::reset() {
    _isFinalized = false;

    // Unregister the block as entry for its continuation blocks
    ProgramBlock* check;
    check = _nonZeroBlock->popEntry();
    assert(check == this);

    if (_zeroBlock != nullptr) {
        check = _zeroBlock->popEntry();
        assert(check == this);
    }
}

void ProgramBlock::finalize(bool isDelta, int amount, int numSteps,
                            ProgramBlock* zeroBlock, ProgramBlock* nonZeroBlock) {
    _isFinalized = true;

    _isDelta = isDelta;
    _instructionAmount = amount;
    _numSteps = numSteps;

    _zeroBlock = zeroBlock;
    if (zeroBlock != nullptr) {
        zeroBlock->pushEntry(this);
    }

    _nonZeroBlock = nonZeroBlock;
    nonZeroBlock->pushEntry(this);
}

void ProgramBlock::dump() {
    dumpWithoutEOL();
    std::cout << std::endl;
}

void ProgramBlock::dumpWithoutEOL() {
    assert(_isFinalized);

    if (_instructionAmount >= 0) {
        std::cout << (_isDelta ? "INC " : "SHR ") << _instructionAmount;
    } else {
        std::cout << (_isDelta ? "DEC " : "SHL ") << -_instructionAmount;
    }
}
