//
//  InterpretedProgramCanonizer.h
//  BusyBeaverFinder
//
//  Created by Erwin on 02/03/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//
#pragma once

#include <map>
#include <vector>

#include "InterpretedProgram.h"
#include "Types.h"
#include "Program.h"
#include "ProgramBlock.h"

class InterpretedProgramCanonizer : public InterpretedProgram {
    std::vector<AdjustableProgramBlock> _blocks;

    // Calculates a canonical start index for a block so that two blocks that behave the same (but
    // may have a different original start index and number of steps), have the same canonical
    // start index.
    int canonicalStartIndexForBlock(const ProgramBlock* block,
                                    const InterpretedProgram& source) const;

    char charForIndex(int index) const;

public:
    InterpretedProgramCanonizer(const InterpretedProgram& source);

    int numProgramBlocks() const override { return static_cast<int>(_blocks.size()); };
    const ProgramBlock* programBlockAt(int index) const override { return &_blocks[index]; };
    const ProgramBlock* getEntryBlock() const override { return &_blocks[0]; };

    void dumpCanonicalProgram(std::ostream &os) const;
    void dumpBlockSizes(std::ostream &os) const;
    std::string canonicalProgramString() const;
    std::string blockSizeString() const;
};
