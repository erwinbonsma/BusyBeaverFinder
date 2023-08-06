//
//  InterpretedProgram.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 26/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "InterpretedProgramBuilder.h"

#include <assert.h>
#include <iostream>

#include "Utils.h"
#include "Program.h"

// Set when instruction has been set
const uint8_t INSTRUCTION_SET_BIT = 0x01;

// Set when instruction is a Delta (otherwise it is a Shift)
const uint8_t INSTRUCTION_TYPE_BIT = 0x02;

InterpretedProgramBuilder::InterpretedProgramBuilder() {
    _stateP = _state;

    for (int i = maxProgramBlocks; --i >=0; ) {
        _blockIndexLookup[i] = -1;
    }

    _stateP->numBlocks = 0;
    _stateP->numFinalizedBlocks = 0;
    enterBlock(InstructionPointer { .col = 0, .row = 0 }, TurnDirection::COUNTERCLOCKWISE);
}

void InterpretedProgramBuilder::checkState() {
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


void InterpretedProgramBuilder::push() {
    ProgramStack* oldStateP = _stateP++;

    assert(_stateP < (_state + maxProgramStackFrames));

    // Copy state
    *_stateP = *oldStateP;

//    checkState();
}

void InterpretedProgramBuilder::pop() {
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

InstructionPointer InterpretedProgramBuilder::startInstructionForBlock(ProgramBlock* block) {
    int val = block->getStartIndex() >> 1;
    int col = val % maxWidth;
    int row = (val - col) / maxWidth;

    return InstructionPointer { .col = col, .row = row };
}

TurnDirection InterpretedProgramBuilder::startTurnDirectionForBlock(ProgramBlock* block) {
    return (TurnDirection)(block->getStartIndex() & 0x01);
}

ProgramPointer InterpretedProgramBuilder::getStartProgramPointer(ProgramBlock* block,
                                                                 Program& program) {
    ProgramPointer pp;

    pp.p = startInstructionForBlock(block);
    pp.dir = (Dir)0;

    // Find a TURN
    while (program.getInstruction(nextInstructionPointer(pp)) != Ins::TURN) {
        pp.dir = (Dir)((int)pp.dir + 1);
    }

    return pp;
}

ProgramBlock* InterpretedProgramBuilder::getBlock(InstructionPointer insP, TurnDirection turn) {
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

void InterpretedProgramBuilder::setInstruction(bool isDelta) {
    _stateP->activeBlock.flags |= INSTRUCTION_SET_BIT;
    if (isDelta) {
        _stateP->activeBlock.flags |= INSTRUCTION_TYPE_BIT;
    } else {
        _stateP->activeBlock.flags &= ~INSTRUCTION_TYPE_BIT;
    }
}

bool InterpretedProgramBuilder::isInstructionSet() {
    return (_stateP->activeBlock.flags & INSTRUCTION_SET_BIT) != 0;
}

bool InterpretedProgramBuilder::isDeltaInstruction() {
    assert(isInstructionSet());
    return (_stateP->activeBlock.flags & INSTRUCTION_TYPE_BIT) != 0;
}

int InterpretedProgramBuilder::getAmount() {
    return _stateP->activeBlock.amount;
}

int InterpretedProgramBuilder::getNumSteps() {
    return _stateP->activeBlock.numSteps;
}

ProgramBlock* InterpretedProgramBuilder::finalizeBlock(InstructionPointer endP) {
    ProgramBlock* block = &_blocks[_stateP->activeBlockIndex];

    assert( !block->isFinalized() );

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

ProgramBlock* InterpretedProgramBuilder::enterBlock(ProgramBlock* block) {
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

ProgramBlock* InterpretedProgramBuilder::enterBlock(InstructionPointer startP,
                                                    TurnDirection turnDir) {
    return enterBlock(getBlock(startP, turnDir));
}
