//
//  InterpretedProgramCanonizer.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 02/03/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//

#include "InterpretedProgramCanonizer.h"

#include <map>
#include <deque>

int InterpretedProgramCanonizer::canonicalStartIndexForBlock(const ProgramBlock* block,
                                                             const InterpretedProgram& prog) const
{
    if (!block->isFinalized()) {
        return -1;
    }
    if (block->isExit()) {
        return -2;
    }
    if (block->isHang()) {
        return -3;
    }

    int value = 0;
    if (block->zeroBlock()) {
        value |= prog.indexOf(block->zeroBlock()) & 0xff;
    }
    value <<= 8;

    if (block->nonZeroBlock()) {
        value |= prog.indexOf(block->nonZeroBlock()) & 0xff;
    }
    value <<= 6;

    value |= abs(block->getInstructionAmount()) & 0x3f;
    value <<= 1;

    value |= ((block->getInstructionAmount() > 0) & 0x1);
    value <<= 1;

    value |= (block->isDelta() & 0x1);
    return value;
}

InterpretedProgramCanonizer::InterpretedProgramCanonizer(const InterpretedProgram& source) {
    // Maps from "my" startIndex to block index
    std::map<int, int> myMap;

    // Maps from their startIndex to block index
    std::map<int, int> theirMap;

    std::deque<const ProgramBlock*> pending;
    pending.push_back(source.getEntryBlock());

    // First identify all the unique blocks, and put them in canonical order
    while (!pending.empty()) {
        const ProgramBlock* block = pending.front();
        pending.pop_front();

        int startIndex = canonicalStartIndexForBlock(block, source);
        auto result = myMap.find(startIndex);
        if (result == myMap.end()) {
            int index = static_cast<int>(_blocks.size());
            // Block does not yet exist. Add it
            _blocks.emplace_back(startIndex);

            myMap.insert({startIndex, index});
            theirMap.insert({block->getStartIndex(), index});
        } else {
            // Similar block already exists. Link to that.
            theirMap.insert({block->getStartIndex(), result->second});
        }

        if (
            block->zeroBlock() &&
            theirMap.find(block->zeroBlock()->getStartIndex()) == theirMap.end()
        ) {
            pending.push_back(block->zeroBlock());
        }
        if (
            block->nonZeroBlock() &&
            theirMap.find(block->nonZeroBlock()->getStartIndex()) == theirMap.end()
        ) {
            pending.push_back(block->nonZeroBlock());
        }
    }

    // Next, finalize all blocks. This is possible now that a canonical block should exist for all
    // (reachable) blocks.
    for (int i = 0; i < source.numProgramBlocks(); ++i) {
        const ProgramBlock* srcBlock = source.programBlockAt(i);

        auto result = theirMap.find(srcBlock->getStartIndex());
        if (result == theirMap.end()) {
            continue;
        }

        AdjustableProgramBlock* dstBlock = &_blocks[result->second];

        if (dstBlock->isFinalized()) {
            dstBlock->updateNumSteps(std::min(dstBlock->getNumSteps(),
                                              srcBlock->getNumSteps()));
            continue;
        }

        if (!srcBlock->isFinalized()) {
            continue;
        }

        if (srcBlock->isExit()) {
            dstBlock->finalizeExit(srcBlock->getNumSteps());
            continue;
        }

        if (srcBlock->isHang()) {
            dstBlock->finalizeHang();
            continue;
        }

        auto getMyBlockFn = [&](const ProgramBlock* theirBlock) {
            return (theirBlock
                    ? &_blocks[theirMap.find(theirBlock->getStartIndex())->second]
                    : nullptr);
        };

        dstBlock->finalize(srcBlock->isDelta(),
                           srcBlock->getInstructionAmount(),
                           srcBlock->getNumSteps(),
                           getMyBlockFn(srcBlock->zeroBlock()),
                           getMyBlockFn(srcBlock->nonZeroBlock()));
    }
}
