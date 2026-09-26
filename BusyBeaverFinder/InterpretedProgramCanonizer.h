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

    // Makes the supplied program more canonical. Occassionally, a program requires more than one
    // canonization step before it is fully canonized. This can happen when program blocks are
    // merged. This in turn can result in the merge of program blocks that jumped to blocks that
    // were merged.
    //
    // To hide these implementation details, this constructor is not directly exposed. Instead it
    // it exposed via "canonizeProgram" which will recursively canonize a program until the input
    // and output are unchanged.
    InterpretedProgramCanonizer(const InterpretedProgram& source);

public:
    // Do not allow copy construction/assignment.
    // The raw pointers used in ProgramBlock do not support this.
    InterpretedProgramCanonizer(const InterpretedProgramCanonizer&) = delete;
    InterpretedProgramCanonizer& operator=(const InterpretedProgramCanonizer&) = delete;

    // Move construction/assignment is allowed.
    InterpretedProgramCanonizer(InterpretedProgramCanonizer&&) noexcept = default;
    InterpretedProgramCanonizer& operator=(InterpretedProgramCanonizer&&) = default;

    static InterpretedProgramCanonizer canonizeProgram(const InterpretedProgram& source);

    int numProgramBlocks() const override { return static_cast<int>(_blocks.size()); };
    const ProgramBlock* programBlockAt(int index) const override { return &_blocks[index]; };
};
