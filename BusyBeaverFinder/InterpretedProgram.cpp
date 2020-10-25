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

    if (block->constZeroBlock() != nullptr) {
        os << indexOf(block->constZeroBlock());
    } else {
        os << "-";
    }

    os << "/" << indexOf(block->constNonZeroBlock());

    os << ", #Steps = " << block->getNumSteps();

    os << ", Entries[";
    for (int i = 0; i < block->numEntryBlocks(); i++) {
        if (i != 0) {
            os << ", ";
        }
        os << indexOf(block->constEntryBlock(i));
    }
    os << "]";
}

void InterpretedProgram::dump() const {
    for (int i = 0; i < numProgramBlocks(); ++i) {
        dumpBlock(programBlockAt(i), std::cout);

        std::cout << std::endl;
    }
}
