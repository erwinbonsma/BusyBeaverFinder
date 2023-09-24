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
#include <vector>

#include "Utils.h"
#include "Program.h"

// Set when instruction has been set
const uint8_t INSTRUCTION_SET_BIT = 0x01;

// Set when instruction is a Delta (otherwise it is a Shift)
const uint8_t INSTRUCTION_TYPE_BIT = 0x02;

InterpretedProgramBuilder InterpretedProgramBuilder::fromProgram(Program& program) {
    InterpretedProgramBuilder  builder;
    std::vector<const ProgramBlock*> stack;

    while (true) {
        const ProgramBlock* block = builder.buildActiveBlock(program);

        // Add new continuation blocks to stack
        if (block && !block->isExit()) {
            if (block->nonZeroBlock() && !block->nonZeroBlock()->isFinalized()) {
                stack.push_back(block->nonZeroBlock());
            }
            if (block->zeroBlock() && !block->zeroBlock()->isFinalized()) {
                stack.push_back(block->zeroBlock());
            }
        }

        // Select a new block to finalize. Popped blocks may meanwhile be finalized as
        // they can be added to the stack more than once.
        do {
            if (stack.empty()) return builder;

            block = stack.back();
            stack.pop_back();
        } while (block->isFinalized());

        builder.enterBlock(block);
    }

    return builder;
}

InterpretedProgramBuilder::InterpretedProgramBuilder() {
    _stateP = _state;

    for (int i = maxProgramBlocks; --i >=0; ) {
        _blockIndexLookup[i] = -1;
    }

    _stateP->numBlocks = 0;
    _stateP->numFinalizedBlocks = 0;
    enterBlock(InstructionPointer { .col = 0, .row = 0 }, TurnDirection::COUNTERCLOCKWISE);
}

void InterpretedProgramBuilder::addDataInstruction(Dir dir) {
    MutableProgramBlock &activeBlock = _stateP->activeBlock;

    activeBlock.flags |= INSTRUCTION_SET_BIT;
    switch (dir) {
        case Dir::UP:
        case Dir::DOWN:
            activeBlock.flags |= INSTRUCTION_TYPE_BIT;
            activeBlock.amount += 1 - (int)dir;
            break;
        case Dir::RIGHT:
        case Dir::LEFT:
            activeBlock.flags &= ~INSTRUCTION_TYPE_BIT;
            activeBlock.amount += 2 - (int)dir;
            break;
    }
}

const ProgramBlock* InterpretedProgramBuilder::buildActiveBlock(Program& program) {
    ProgramBlock *block = _blocks + _stateP->activeBlockIndex;
    MutableProgramBlock &activeBlock = _stateP->activeBlock;

    ProgramPointer pp = getStartProgramPointer(block, program);
    TurnDirection td = turnDirectionForBlock(block);

    do {
        InstructionPointer ip;
        Ins ins;
        do {
            ip = nextInstructionPointer(pp);
            ins = program.getInstruction(ip);

            switch (ins) {
                case Ins::DONE:
                    activeBlock.numSteps++;
                    return finalizeExitBlock();
                case Ins::UNSET:
                    return nullptr;
                case Ins::NOOP:
                    break;
                case Ins::DATA:
                    addDataInstruction(pp.dir);
                    break;
                case Ins::TURN:
                    if (activeBlock.flags & INSTRUCTION_SET_BIT) {
                        return finalizeBlock(pp.p);
                    } else {
                        if (td == TurnDirection::COUNTERCLOCKWISE) {
                            pp.dir = (Dir)(((int)pp.dir + 3) % 4);
                        } else {
                            pp.dir = (Dir)(((int)pp.dir + 1) % 4);
                        }
                    }
            }
        } while (ins == Ins::TURN);

        pp.p = ip;
    } while (activeBlock.numSteps++ < 127);

    return finalizeHangBlock();
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

InstructionPointer InterpretedProgramBuilder::startInstructionForBlock(const ProgramBlock* block) {
    int val = block->getStartIndex() >> 1;
    int8_t col = val % maxWidth;
    int8_t row = (val - col) / maxWidth;

    return InstructionPointer { .col = col, .row = row };
}

TurnDirection InterpretedProgramBuilder::turnDirectionForBlock(const ProgramBlock* block) {
    return (TurnDirection)(block->getStartIndex() & 0x01);
}

ProgramPointer InterpretedProgramBuilder::getStartProgramPointer(const ProgramBlock* block,
                                                                 Program& program) {
    ProgramPointer pp;

    if (block == getEntryBlock()) {
        pp.p = InstructionPointer { .col = 0, .row = -1 };
        pp.dir = Dir::UP;
    } else {
        pp.p = startInstructionForBlock(block);
        pp.dir = (Dir)0;

        // Find a TURN
        while (program.getInstruction(nextInstructionPointer(pp)) != Ins::TURN) {
            pp.dir = (Dir)((int)pp.dir + 1);
        }
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

const ProgramBlock* InterpretedProgramBuilder::finalizeExitBlock() {
    ProgramBlock* block = &_blocks[_stateP->activeBlockIndex];
    assert( !block->isFinalized() );

    block->finalizeExit(getNumSteps());
    _finalizedStack[ _stateP->numFinalizedBlocks++ ] = (int)(block - _blocks);

    return block;
}

const ProgramBlock* InterpretedProgramBuilder::finalizeHangBlock() {
    ProgramBlock* block = &_blocks[_stateP->activeBlockIndex];
    assert( !block->isFinalized() );

    block->finalizeHang();
    _finalizedStack[ _stateP->numFinalizedBlocks++ ] = (int)(block - _blocks);

    return block;
}

const ProgramBlock* InterpretedProgramBuilder::finalizeBlock(InstructionPointer endP) {
    ProgramBlock* block = &_blocks[_stateP->activeBlockIndex];
    assert( !block->isFinalized() );

    ProgramBlock* zeroBlock = nullptr;
    if (
        turnDirectionForBlock(block) == TurnDirection::COUNTERCLOCKWISE &&
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

const ProgramBlock* InterpretedProgramBuilder::enterBlock(const ProgramBlock* block) {
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

const ProgramBlock* InterpretedProgramBuilder::enterBlock(InstructionPointer startP,
                                                          TurnDirection turnDir) {
    return enterBlock(getBlock(startP, turnDir));
}
