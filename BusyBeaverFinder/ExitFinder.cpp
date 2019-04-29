//
//  ExitFinder.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "ExitFinder.h"

#include <iostream>

#include "Utils.h"

// Limit on the recursion level of the isPossibleExitValue method. Its a simple guard against
// infinite recursion without explicitly checking if it has entered a loop.
const int maxBacktrackDepth = 4;

ExitFinder::ExitFinder(Program& program, InterpretedProgram& interpretedProgram) :
    _program(program),
    _interpretedProgram(interpretedProgram)
{
    for (int i = 0; i < maxProgramBlocks; i++) {
        _visited[i] = false;
    }

    // Bound on the maximum number of steps to execute within a block without processing a DATA
    // instruction before concluding that the block loops into itself. This is not an exact bound
    // but should be high enough. (If not, it will be discovered as this program itself will end up
    // in an endless loop).
    _maxSteps = (program.getWidth() - 1) * (program.getHeight() - 1);
}

bool ExitFinder::isPossibleExitValue(ProgramBlock* block, bool zeroValue, int delta, int depth) {
    if ( !block->isDelta() ) {
        // The block performs a shift. We cannot conclude anything about its exit value
        return true;
    }

    if (depth++ > maxBacktrackDepth) {
        return true;
    }

    delta -= block->getInstructionAmount();

    for (int i = block->numEntryBlocks(); --i >= 0; ) {
        ProgramBlock* entryBlock = block->entryBlock(i);

        if (entryBlock->nonZeroBlock() == block) {
            if (zeroValue && delta == 0) {
                return false;
            }

            if (isPossibleExitValue(entryBlock, zeroValue, delta, depth)) {
                return true;
            }
        }

        if (entryBlock->zeroBlock() == block) {
            // The entry value when coming from this block is zero.
            if (zeroValue == (delta == 0)) {
                return true;
            }
        }
    }

    if (block->numEntryBlocks() == 0) {
        // This is the very first block of the program, whose entry value is always zero
        return (zeroValue == (delta == 0));
    }

    return false;
}

bool ExitFinder::isReachable(ProgramBlock* block) {
    for (int i = block->numEntryBlocks(); --i >= 0; ) {
        ProgramBlock* entryBlock = block->entryBlock(i);

        if (entryBlock->nonZeroBlock() == block) {
            if (isPossibleExitValue(entryBlock, false, 0, 0)) {
                return true;
            };
        }

        if (entryBlock->zeroBlock() == block) {
            if (isPossibleExitValue(entryBlock, true, 0, 0)) {
                return true;
            };
        }
    }

    return false;
}

bool ExitFinder::finalizeBlock(ProgramBlock* block) {
    _interpretedProgram.enterBlock(block);

    ProgramPointer pp = _interpretedProgram.getStartProgramPointer(block, _program);
    int steps = 0;

    // Rotation delta
    int delta = (_interpretedProgram.startTurnDirectionForBlock(block) == TurnDirection::CLOCKWISE)
        ? 1 : 3;

    while (1) {
processInstruction:
        InstructionPointer insP = nextInstructionPointer(pp);

        switch (_program.getInstruction(insP)) {
            case Ins::DATA:
                switch (pp.dir) {
                    case Dir::UP:
                        _interpretedProgram.setInstruction(true);
                        _interpretedProgram.incAmount();
                        break;
                    case Dir::DOWN:
                        _interpretedProgram.setInstruction(true);
                        _interpretedProgram.decAmount();
                        break;
                    case Dir::RIGHT:
                        _interpretedProgram.setInstruction(false);
                        _interpretedProgram.incAmount();
                        break;
                    case Dir::LEFT:
                        _interpretedProgram.setInstruction(false);
                        _interpretedProgram.decAmount();
                        break;
                }
                break;
            case Ins::NOOP:
                break;
            case Ins::DONE:
            case Ins::UNSET:
                // Escaped from loop. So cannot conclude that program hangs
                return false;
            case Ins::TURN:
                if (_interpretedProgram.isInstructionSet()) {
                    _interpretedProgram.finalizeBlock(pp.p);
                    return true;
                } else {
                    pp.dir = (Dir)(((int)pp.dir + delta) % 4);
                    goto processInstruction;
                }
                break;
        }

        if (steps++ > _maxSteps) {
            // Apparently the block itself is in an endless loop. For purposes of No Exit hang
            // detection, just finalize it. We cannot conclude here that the program hangs, as there
            // is no guarantee that this block will be entered.
            //
            // Note, setting an instruction so that the next TURN will finalize the block. This
            // ensures that the block is finalized at a TURN, as it should.
            _interpretedProgram.setInstruction(true);
        }
        _interpretedProgram.incSteps();
        pp.p = insP;
    }
}

bool ExitFinder::visitBlock(ProgramBlock* block) {
    if (block == nullptr) {
        // This is a block that cannot be entered
        return false;
    }

    if (_visited[block->getStartIndex()]) {
        // Already visited this block
        return false;
    }

    _visited[block->getStartIndex()] = true;

    if (!block->isFinalized()) {
        if (!finalizeBlock(block)) {
            _exits[_numExits++] = block;

            return false;
        }
    }

    // Add to stack of blocks whose children should be visited
    *(_topP++) = block;

    return false;
}

bool ExitFinder::canExitFrom(ProgramBlock* block) {
    _nextP = _pendingStack;
    _topP = _pendingStack;

    _numExits = 0;

    visitBlock(block);
    while (_nextP < _topP) {
        ProgramBlock* block = *_nextP++;

        visitBlock(block->zeroBlock());
        visitBlock(block->nonZeroBlock());
    }

    // Reset tracking state for finalized blocks
    while (_topP > _pendingStack) {
        _visited[ (*--_topP)->getStartIndex() ] = false;
    }

    bool canEscape = false;

    for (int i = _numExits; --i >= 0; ) {
        if (isReachable(_exits[i])) {
            canEscape = true;
        }

        // Reset tracking state for exit
        _visited[ _exits[i]->getStartIndex() ] = false;
    }

    return canEscape;
}
