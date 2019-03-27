//
//  CompiledProgram.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 26/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "CompiledProgram.h"

#include <iostream>

// Set when instruction has been set
const unsigned char INSTRUCTION_SET_BIT = 0x01;

// Set when instruction is a Delta (otherwise it is a Shift)
const unsigned char INSTRUCTION_TYPE_BIT = 0x02;

CompiledProgram::CompiledProgram() {
    _stateP = _state;

    for (int i = maxProgramBlocks; --i >=0; ) {
        _blockIndexLookup[i] = -1;
    }

    _stateP->numBlocks = 0;
    enterBlock(InstructionPointer { .col = 0, .row = 0 }, TurnDirection::COUNTERCLOCKWISE);
}

void CompiledProgram::push() {
    ProgramStack* oldStateP = _stateP++;

    assert(_stateP < (_state + maxProgramStackFrames));

    // Copy state
    *_stateP = *oldStateP;
}

void CompiledProgram::pop() {
    assert(_stateP > _state);

    int oldNumBlocks = _stateP->numBlocks;

    // Pop state from stack
    _stateP--;

    // Remove entries to blocks that do not exist anymore from look-up
    for (int i = oldNumBlocks; --i >= _stateP->numBlocks; ) {
        _blockIndexLookup[ _blocks[i].getStartIndex() ] = -1;
    }
}

ProgramBlock* CompiledProgram::getBlock(InstructionPointer insP, TurnDirection turn) {
    int lookupIndex = ((insP.col + insP.row * maxWidth) << 1) + (int)turn;
    int blockIndex = _blockIndexLookup[lookupIndex];

    // Block already exists for this starting point
    if (blockIndex >= 0) {
        return _blocks + blockIndex;
    }

    // Allocate block
    ProgramBlock* block = _blocks + _stateP->numBlocks++;
    _blockIndexLookup[lookupIndex] = (int)(block - _blocks);

    // Initialize it
    block->init(lookupIndex);

    return block;
}

void CompiledProgram::setInstruction(bool isDelta) {
    _stateP->activeBlock.flags |= INSTRUCTION_SET_BIT;
    if (isDelta) {
        _stateP->activeBlock.flags |= INSTRUCTION_TYPE_BIT;
    } else {
        _stateP->activeBlock.flags &= ~INSTRUCTION_TYPE_BIT;
    }
}

bool CompiledProgram::isInstructionSet() {
    return (_stateP->activeBlock.flags & INSTRUCTION_SET_BIT) != 0;
}

bool CompiledProgram::isDeltaInstruction() {
    assert(isInstructionSet());
    return (_stateP->activeBlock.flags & INSTRUCTION_TYPE_BIT) != 0;
}

int CompiledProgram::getAmount() {
    return _stateP->activeBlock.amount;
}

int CompiledProgram::getNumSteps() {
    return _stateP->activeBlock.numSteps;
}

void CompiledProgram::finalizeBlock(InstructionPointer endP) {
    ProgramBlock* block = &_blocks[_stateP->activeBlockIndex];
    ProgramBlock* zeroBlock = getBlock(endP, TurnDirection::COUNTERCLOCKWISE);
    ProgramBlock* nonZeroBlock = getBlock(endP, TurnDirection::CLOCKWISE);

    block->finalize(isDeltaInstruction(), getAmount(), getNumSteps(), zeroBlock, nonZeroBlock);
}

ProgramBlock* CompiledProgram::enterBlock(InstructionPointer startP, TurnDirection turnDir) {
    ProgramBlock* block = getBlock(startP, turnDir);

    // Reset state to enable construction (or stepping through) this block
    _stateP->activeBlock.flags = 0;
    _stateP->activeBlock.amount = 0;
    _stateP->activeBlock.numSteps = 0;
    _stateP->activeBlockIndex = (int)(block - _blocks);

    return block;
}

void CompiledProgram::dump() {
    for (int i = 0; i < _stateP->numBlocks; i++) {
        ProgramBlock* block = _blocks + i;

        std::cout << i << " (" << block->getStartIndex() << "): ";

        if (!block->isFinalized()) {
            std::cout << "-";
        } else {
            int amount = block->getInstructionAmount();
            if (amount >= 0) {
                std::cout << (block->isDelta() ? "INC " : "SHR ") << amount;
            } else {
                std::cout << (block->isDelta() ? "DEC " : "SHL ") << -amount;
            }

            std::cout << " => "
            << (block->zeroBlock() - _blocks) << "/"
            << (block->nonZeroBlock() - _blocks)
            << ", #Steps = " << block->getNumSteps();
        }

        std::cout << std::endl;
    }
}
