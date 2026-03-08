//
//  InterpretedProgram.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 25/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "InterpretedProgram.h"

void InterpretedProgram::dumpBlock(const ProgramBlock* block, std::ostream &os) const {
    os << indexOf(block) << " (" << block->getStartIndex() << "): ";
    os << *block;

    if (!block->isFinalized()) {
        return;
    }

    os << " => ";

    if (block->zeroBlock() != nullptr) {
        os << indexOf(block->zeroBlock());
    } else {
        os << "-";
    }

    if (block->nonZeroBlock() != nullptr) {
        os << "/" << indexOf(block->nonZeroBlock());
    } else {
        os << "/-";
    }

    os << ", #Steps = " << block->getNumSteps();

    os << ", Entries[";
    for (int i = 0; i < block->numEntryBlocks(); i++) {
        if (i != 0) {
            os << ", ";
        }
        os << indexOf(block->entryBlock(i));
    }
    os << "]";
}

void InterpretedProgram::dump() const {
    size_t numBlocks = numProgramBlocks();
    std::cout << "numBlocks = " << numBlocks << std::endl;
    for (int i = 0; i < numBlocks; ++i) {
        dumpBlock(programBlockAt(i), std::cout);

        std::cout << std::endl;
    }
}


char InterpretedProgram::charForIndex(int index) const {
    assert(index >= 0);
    if (index < 26) {
        return 'a' + index;
    }
    index -= 26;
    if (index < 26) {
        return 'A' + index;
    }
    return '?';
}

void InterpretedProgram::dumpShortProgram(std::ostream &os) const {
    for (int i = 0; i < numProgramBlocks(); ++i) {
        const ProgramBlock* block = programBlockAt(i);
        if (i != 0) {
            os << " ";
        }
        os << charForIndex(indexOf(block));

        if (!block->isFinalized()) {
            os << "?";
            continue;
        }
        if (block->isExit()) {
            os << "X";
            continue;
        }
        if (block->isHang()) {
            os << "H";
            continue;
        }

        os << (block->isDelta()
               ? (block->getInstructionAmount() > 0 ? "+" : "-")
               : (block->getInstructionAmount() > 0 ? ">" : "<"));
        os << abs(block->getInstructionAmount());

        auto charForBlockFn = [this](const ProgramBlock* block) {
            return block ? charForIndex(indexOf(block)) : '-';
        };

        os << charForBlockFn(block->zeroBlock());
        os << charForBlockFn(block->nonZeroBlock());
    }
}

void InterpretedProgram::dumpBlockSizes(std::ostream &os) const {
    for (int i = 0; i < numProgramBlocks(); ++i) {
        const ProgramBlock* block = programBlockAt(i);
        if (i != 0) {
            os << " ";
        }
        os << block->getNumSteps();
    }
}

std::string InterpretedProgram::shortProgramString() const {
    std::ostringstream os;
    dumpShortProgram(os);
    return os.str();
}

std::string InterpretedProgram::blockSizeString() const {
    std::ostringstream os;
    dumpBlockSizes(os);
    return os.str();
}
