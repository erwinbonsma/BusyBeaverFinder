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
