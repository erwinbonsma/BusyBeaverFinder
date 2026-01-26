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
constexpr uint8_t INSTRUCTION_SET_BIT = 0x01;

// Set when instruction is a Delta (otherwise it is a Shift)
constexpr uint8_t INSTRUCTION_TYPE_BIT = 0x02;

InterpretedProgramBuilder::InterpretedProgramBuilder() :
    _blocks(create_indexed_array<ProgramBlock, maxProgramBlocks>())
{
    _finalizedStack.reserve(_blocks.size());
    std::fill(_blockIndexLookup.begin(), _blockIndexLookup.end(), -1);

    reset();
}

void InterpretedProgramBuilder::reset() {
    while (!_state.empty()) {
        pop();
    }

    _state.emplace_back();

    enterBlock(InstructionPointer { .col = 0, .row = 0 }, TurnDirection::COUNTERCLOCKWISE);
}

void InterpretedProgramBuilder::buildFromProgram(Program& program) {
    reset();

    std::vector<const ProgramBlock*> stack;

    while (true) {
        const ProgramBlock* block = buildActiveBlock(program);

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
            if (stack.empty()) return;

            block = stack.back();
            stack.pop_back();
        } while (block->isFinalized());

        enterBlock(block);
    }
}

void InterpretedProgramBuilder::addDataInstruction(Dir dir) {
    MutableProgramBlockProps &props = _state.back().activeProps;

    props.flags |= INSTRUCTION_SET_BIT;
    switch (dir) {
        case Dir::UP:
        case Dir::DOWN:
            props.flags |= INSTRUCTION_TYPE_BIT;
            props.amount += 1 - static_cast<int>(dir);
            break;
        case Dir::RIGHT:
        case Dir::LEFT:
            props.flags &= ~INSTRUCTION_TYPE_BIT;
            props.amount += 2 - static_cast<int>(dir);
            break;
    }
}

const ProgramBlock* InterpretedProgramBuilder::buildActiveBlock(Program& program) {
    ProgramStack &stateP = _state.back();
    ProgramBlock *block = stateP.activeBlock;
    MutableProgramBlockProps &props = stateP.activeProps;

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
                    props.numSteps++;
                    return finalizeExitBlock();
                case Ins::UNSET:
                    return nullptr;
                case Ins::NOOP:
                    break;
                case Ins::DATA:
                    addDataInstruction(pp.dir);
                    break;
                case Ins::TURN:
                    if (props.flags & INSTRUCTION_SET_BIT) {
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
    } while (props.numSteps++ < 127);

    return finalizeHangBlock();
}

//void InterpretedProgramBuilder::checkState() {
//    ProgramStack &stateP = _state.back();
//    for (int i = 0; i < stateP.numBlocks; i++) {
//        ProgramBlock* block = &_blocks[i];
//
//        if (block->isFinalized()) {
//            if (
//                (block->nonZeroBlock() - _blocks.data()) >= stateP.numBlocks ||
//                (block->zeroBlock() - _blocks.data()) >= stateP.numBlocks
//            ) {
//                dump();
//                assert(false);
//            }
//        }
//    }
//}

void InterpretedProgramBuilder::push() {
    _state.emplace_back(_activatedStack.size(), _finalizedStack.size());

//    checkState();
}

void InterpretedProgramBuilder::pop() {
    // Pop state from stack
    ProgramStack &stateP = _state.back();

    // Remove activated blocks
    while (_activatedStack.size() > stateP.numActivatedAtStart) {
        _blockIndexLookup[_activatedStack.back()->getStartIndex()] = -1;
        _activatedStack.pop_back();
    }

    // Reset blocks that are not finalized anymore
    while (_finalizedStack.size() > stateP.numFinalizedAtStart) {
        _finalizedStack.back()->reset();
        _finalizedStack.pop_back();
    }

    _state.pop_back();

//    checkState();
}

InstructionPointer InterpretedProgramBuilder::startInstructionForBlock(const ProgramBlock* block) {
    int val = block->getStartIndex() >> 1;
    int8_t col = val % maxProgramSize;
    int8_t row = (val - col) / maxProgramSize;

    return InstructionPointer { .col = col, .row = row };
}

TurnDirection InterpretedProgramBuilder::turnDirectionForBlock(const ProgramBlock* block) {
    return static_cast<TurnDirection>(block->getStartIndex() & 0x01);
}

ProgramPointer InterpretedProgramBuilder::getStartProgramPointer(const ProgramBlock* block,
                                                                 Program& program) {
    ProgramPointer pp;

    if (block == getEntryBlock()) {
        pp.p = InstructionPointer { .col = 0, .row = -1 };
        pp.dir = Dir::UP;
    } else {
        pp.p = startInstructionForBlock(block);
        pp.dir = static_cast<Dir>(0);

        // Find a TURN
        while (program.getInstruction(nextInstructionPointer(pp)) != Ins::TURN) {
            pp.dir = static_cast<Dir>(static_cast<int>(pp.dir) + 1);
        }
    }

    return pp;
}

ProgramBlock* InterpretedProgramBuilder::getBlock(InstructionPointer insP, TurnDirection turn) {
    int lookupIndex = ((insP.col + insP.row * maxProgramSize) << 1) + static_cast<int>(turn);
    return &_blocks[lookupIndex];
}

bool InterpretedProgramBuilder::isInstructionSet() {
    return (_state.back().activeProps.flags & INSTRUCTION_SET_BIT) != 0;
}

bool InterpretedProgramBuilder::isDeltaInstruction() {
    assert(isInstructionSet());
    return (_state.back().activeProps.flags & INSTRUCTION_TYPE_BIT) != 0;
}

int InterpretedProgramBuilder::getAmount() {
    return _state.back().activeProps.amount;
}

int InterpretedProgramBuilder::getNumSteps() {
    return _state.back().activeProps.numSteps;
}

const ProgramBlock* InterpretedProgramBuilder::finalizeExitBlock() {
    ProgramBlock* block = _state.back().activeBlock;
    assert( !block->isFinalized() );

    block->finalizeExit(getNumSteps());
    _finalizedStack.push_back(block);

    return block;
}

const ProgramBlock* InterpretedProgramBuilder::finalizeHangBlock() {
    ProgramBlock* block = _state.back().activeBlock;
    assert( !block->isFinalized() );

    block->finalizeHang();
    _finalizedStack.push_back(block);

    return block;
}

const ProgramBlock* InterpretedProgramBuilder::finalizeBlock(InstructionPointer endP) {
    ProgramBlock* block = _state.back().activeBlock;
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
    _finalizedStack.push_back(block);

//    checkState();

    return block;
}

const ProgramBlock* InterpretedProgramBuilder::enterBlock(const ProgramBlock* block) {
    ProgramStack &stateP = _state.back();
    stateP.activeBlock = &_blocks[block->getStartIndex()]; // Look-up non-const instance

    if (!block->isFinalized()) {
        // Reset state to enable construction (or stepping through) this block
        stateP.activeProps.flags = 0;
        stateP.activeProps.amount = 0;
        stateP.activeProps.numSteps = 0;

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
