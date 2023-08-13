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
    _numEntries = 0;
    _zeroBlock = nullptr;
    _nonZeroBlock = nullptr;
}

void ProgramBlock::reset() {
    _isFinalized = false;

    // Unregister the block as entry for its continuation blocks
    ProgramBlock* check;
    if (_nonZeroBlock != nullptr) {
        check = _nonZeroBlock->popEntry();
        assert(check == this);
        _nonZeroBlock = nullptr;
    }

    if (_zeroBlock != nullptr) {
        check = _zeroBlock->popEntry();
        assert(check == this);
        _zeroBlock = nullptr;
    }
}

void ProgramBlock::finalizeHang() {
    _isFinalized = true;
    _instructionAmount = 0;
    _numSteps = -1;
}

void ProgramBlock::finalizeExit(int numSteps) {
    _isFinalized = true;
    _numSteps = numSteps;
    _instructionAmount = 0;
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

void ProgramBlock::dump() const {
    std::cout << *this << std::endl;
}

std::ostream &operator<<(std::ostream &os, const ProgramBlock &pb) {
    if (!pb.isFinalized()) {
        os << "-";
    } else if (pb.isExit()) {
        os << "EXIT";
    } else if (pb.isHang()) {
        os << "HANG";
    } else if (pb.getInstructionAmount() >= 0) {
        os << (pb.isDelta() ? "INC " : "SHR ") << pb.getInstructionAmount();
    } else {
        os << (pb.isDelta() ? "DEC " : "SHL ") << -pb.getInstructionAmount();
    }

    return os;
}

