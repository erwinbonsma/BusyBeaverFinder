//
//  CompiledProgram.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 26/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "CompiledProgram.h"

#include <assert.h>
#include <iostream>

#include "Utils.h"
#include "Program.h"

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
    _stateP->numFinalizedBlocks = 0;
    enterBlock(InstructionPointer { .col = 0, .row = 0 }, TurnDirection::COUNTERCLOCKWISE);
}

void CompiledProgram::checkState() {
    for (int i = 0; i < _stateP->numBlocks; i++) {
        ProgramBlock* block = _blocks + i;

        if (block->isFinalized()) {
            if (
                (block->nonZeroBlock() - _blocks) >= _stateP->numBlocks ||
                (block->zeroBlock() - _blocks) >= _stateP->numBlocks
            ) {
                dump();
                assert(false);
            }
        }
    }
}


void CompiledProgram::push() {
    ProgramStack* oldStateP = _stateP++;

    assert(_stateP < (_state + maxProgramStackFrames));

    // Copy state
    *_stateP = *oldStateP;

//    checkState();
}

void CompiledProgram::pop() {
    assert(_stateP > _state);

    // Pop state from stack
    ProgramStack* oldStateP = _stateP--;

    // Remove entries to blocks that do not exist anymore from look-up
    for (int i = oldStateP->numBlocks; --i >= _stateP->numBlocks; ) {
        _blockIndexLookup[ _blocks[i].getStartIndex() ] = -1;
    }

    // Reset blocks that are not finalized anymore
    for (int i = oldStateP->numFinalizedBlocks; --i >= _stateP->numFinalizedBlocks; ) {
        _blocks[ _finalizedStack[i] ].reset();
    }

//    checkState();
}

InstructionPointer CompiledProgram::startInstructionForBlock(ProgramBlock* block) {
    int val = block->getStartIndex() >> 1;
    int col = val % maxWidth;
    int row = (val - col) / maxWidth;

    return InstructionPointer { .col = col, .row = row };
}

TurnDirection CompiledProgram::startTurnDirectionForBlock(ProgramBlock* block) {
    return (TurnDirection)(block->getStartIndex() & 0x01);
}

ProgramPointer CompiledProgram::getStartProgramPointer(ProgramBlock* block, Program& program) {
    ProgramPointer pp;

    pp.p = startInstructionForBlock(block);
    pp.dir = (Dir)0;

    // Find a TURN
    while (program.getInstruction(nextInstructionPointer(pp)) != Ins::TURN) {
        pp.dir = (Dir)((int)pp.dir + 1);
    }

    return pp;
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

//    checkState();

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

ProgramBlock* CompiledProgram::finalizeBlock(InstructionPointer endP) {
    ProgramBlock* block = &_blocks[_stateP->activeBlockIndex];

    if (block->isFinalized()) {
        // TODO: Change into assertion failure once finalized blocks are executed
        return nullptr;
    }

    ProgramBlock* zeroBlock = nullptr;
    if (
        startTurnDirectionForBlock(block) == TurnDirection::COUNTERCLOCKWISE &&
        isDeltaInstruction() &&
        getAmount() != 0
    ) {
        // Cannot exit via the zero block, as we entered with a zero value and changed it
    } else {
        zeroBlock = getBlock(endP, TurnDirection::COUNTERCLOCKWISE);
    }

    ProgramBlock* nonZeroBlock = getBlock(endP, TurnDirection::CLOCKWISE);

    block->finalize(isDeltaInstruction(), getAmount(), getNumSteps(), zeroBlock, nonZeroBlock);

    _finalizedStack[ _stateP->numFinalizedBlocks++ ] = (int)(block - _blocks);

//    checkState();

    return block;
}

ProgramBlock* CompiledProgram::enterBlock(ProgramBlock* block) {
    _stateP->activeBlockIndex = (int)(block - _blocks);

    if (!block->isFinalized()) {
        // Reset state to enable construction (or stepping through) this block
        _stateP->activeBlock.flags = 0;
        _stateP->activeBlock.amount = 0;
        _stateP->activeBlock.numSteps = 0;

        // Return null to indicate that block cannot yet be used
        return nullptr;
    }

//    checkState();

    return block;
}

ProgramBlock* CompiledProgram::enterBlock(InstructionPointer startP, TurnDirection turnDir) {
    return enterBlock(getBlock(startP, turnDir));
}

void CompiledProgram::dumpBlock(ProgramBlock* block) {
    std::cout << (block - _blocks) << " (" << block->getStartIndex() << "): ";

    if (!block->isFinalized()) {
        std::cout << "-";
    } else {
        int amount = block->getInstructionAmount();
        if (amount >= 0) {
            std::cout << (block->isDelta() ? "INC " : "SHR ") << amount;
        } else {
            std::cout << (block->isDelta() ? "DEC " : "SHL ") << -amount;
        }

        std::cout << " => ";

        if (block->zeroBlock() != nullptr) {
            std::cout << (block->zeroBlock() - _blocks);
        } else {
            std::cout << "-";
        }

        std::cout << "/" << (block->nonZeroBlock() - _blocks)
        << ", #Steps = " << block->getNumSteps();
    }

    std::cout << std::endl;
}

void CompiledProgram::dump() {
    for (int i = 0; i < _stateP->numBlocks; i++) {
        dumpBlock(_blocks + i);
    }
}
