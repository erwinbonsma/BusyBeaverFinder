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
#include <sstream>

const char* INDEX_CHARS = "0123456789abcdefghijklmnopqrstuvwxyz";

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

    return (((prog.indexOf(block->zeroBlock()) & 0xff) << 16) |
            ((prog.indexOf(block->nonZeroBlock()) & 0xff) << 8) |
            ((abs(block->getInstructionAmount()) & 0x3f) << 2) |
            (block->getInstructionAmount() > 0 ? 0x2 : 0x0) |
            (block->isDelta() ? 0x1 : 0x0));
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

        ProgramBlock* dstBlock = &_blocks[result->second];

        if (dstBlock->isFinalized()) {
            continue;
        }

        if (!srcBlock->isFinalized()) {
            continue;
        }

        if (srcBlock->isExit()) {
            dstBlock->finalizeExit(0);
            continue;
        }

        if (srcBlock->isHang()) {
            dstBlock->finalizeHang();
            continue;
        }

        int startIndex1 = srcBlock->zeroBlock()->getStartIndex();
        int startIndex2 = srcBlock->nonZeroBlock()->getStartIndex();

        dstBlock->finalize(srcBlock->isDelta(),
                           srcBlock->getInstructionAmount(),
                           0,
                           &_blocks[theirMap.find(startIndex1)->second],
                           &_blocks[theirMap.find(startIndex2)->second]);
    }
}

char InterpretedProgramCanonizer::charForIndex(int index) const {
    assert(index >= 0);
    if (index < 10) {
        return '0' + index;
    }
    index -= 10;
    if (index < 26) {
        return 'a' + index;
    }
    return '?';
}

void InterpretedProgramCanonizer::dumpCanonicalProgram(std::ostream &os) const {
    bool isFirst = true;
    for (auto& block : _blocks) {
        if (isFirst) {
            isFirst = false;
        } else {
            os << " ";
        }
        os << charForIndex(indexOf(&block));

        if (block.isExit()) {
            os << "EXIT";
            continue;
        }
        if (block.isHang()) {
            os << "HANG";
            continue;
        }
        if (!block.isFinalized()) {
            os << "???";
            continue;
        }

        os << (block.isDelta()
               ? (block.getInstructionAmount() > 0 ? "+" : "-")
               : (block.getInstructionAmount() > 0 ? ">" : "<"));
        os << abs(block.getInstructionAmount());
        os << "_";
        os << charForIndex(indexOf(block.zeroBlock()));
        os << charForIndex(indexOf(block.nonZeroBlock()));
    }
}

std::string InterpretedProgramCanonizer::canonicalProgramString() const {
    std::ostringstream os;
    dumpCanonicalProgram(os);
    return os.str();
}
