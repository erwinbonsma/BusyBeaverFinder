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

ExitFinder::ExitFinder(Program& program, InterpretedProgramBuilder& interpretedProgramBuilder) :
    _program(program),
    _programBuilder(interpretedProgramBuilder)
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

bool ExitFinder::isPossibleExitValue(const ProgramBlock* block, bool zeroValue, int delta,
                                     int depth) {
    if ( !block->isDelta() ) {
        // The block performs a shift. We cannot conclude anything about its exit value
        return true;
    }

    if (depth++ > maxBacktrackDepth) {
        return true;
    }

    delta -= block->getInstructionAmount();

    for (int i = block->numEntryBlocks(); --i >= 0; ) {
        const ProgramBlock* entryBlock = block->entryBlock(i);

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

bool ExitFinder::isReachable(const ProgramBlock* block) {
    for (int i = block->numEntryBlocks(); --i >= 0; ) {
        const ProgramBlock* entryBlock = block->entryBlock(i);

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

void ExitFinder::visitBlock(const ProgramBlock* block) {
    if (block == nullptr                         // This is a block that cannot be entered
        || _visited[block->getStartIndex()]) {   // Already visited this block
        return;
    }

    std::cout << "visitBlock: ";
    block->dump();

    _visited[block->getStartIndex()] = true;

    if (!block->isFinalized()) {
        _programBuilder.enterBlock(block);
        const ProgramBlock *finalizedBlock = _programBuilder.buildActiveBlock(_program);

        if (finalizedBlock == nullptr) {
            // Can escape from this block
            _exits[_numExits++] = block;
            return;
        }
        if (block->isExit()) {
            _exits[_numExits++] = finalizedBlock;
            return;
        }
        if (block->isHang()) {
            // No children to visit
            return;
        }
    }

    // Add to stack of blocks whose children should be visited
    *(_topP++) = block;
}

bool ExitFinder::canExitFrom(const ProgramBlock* block) {
    assert(block->isFinalized());
    _program.dump();

    _nextP = _pendingStack;
    _topP = _pendingStack;

    _numExits = 0;

    visitBlock(block);
    while (_nextP < _topP) {
        const ProgramBlock* block = *_nextP++;

        visitBlock(block->zeroBlock());
        visitBlock(block->nonZeroBlock());
    }

    // Reset tracking state for finalized blocks
    while (_topP > _pendingStack) {
        _visited[ (*--_topP)->getStartIndex() ] = false;
    }

    bool canEscape = false;

    for (int i = _numExits; --i >= 0; ) {
        if (!canEscape && isReachable(_exits[i])) {
            canEscape = true;
        }

        // Reset tracking state for exit
        _visited[ _exits[i]->getStartIndex() ] = false;
    }

    return canEscape;
}
