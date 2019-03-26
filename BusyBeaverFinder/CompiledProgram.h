//
//  CompiledProgram.h
//  BusyBeaverFinder
//
//  Created by Erwin on 26/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef CompiledProgram_h
#define CompiledProgram_h

#include <stdio.h>

#include "Types.h"
#include "ProgramBlock.h"

const int maxProgramBlocks = 1024;

struct MutableProgramBlock {
    unsigned char flags;
    char amount;
    char numSteps;
};

class CompiledProgram {
    ProgramBlock _blocks[maxProgramBlocks];

    int _activeBlock;
    int _numBlocks;

    MutableProgramBlock _mutableBlock;

public:
    void push();
    void pop();

    bool isBlockMutable();

    // Change the active program block. Can only be invoked when it is still mutable.
    void incSteps();
    void incAmount();
    void decAmount();
    void setInstruction(bool isDelta);

    bool isDeltaInstruction();
    bool isInstructionSet();

    void finalizeBlock(InstructionPointer endP);

    ProgramBlock* enterBlock(InstructionPointer startP, TurnDirection turnDir);
};


#endif /* CompiledProgram_h */
