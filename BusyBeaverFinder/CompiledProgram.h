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

const int maxProgramBlocks = maxWidth * maxHeight * 2;
const int maxProgramStackFrames = maxWidth * maxHeight;

struct MutableProgramBlock {
    unsigned char flags;
    char amount;
    char numSteps;
};

struct ProgramStack {
    MutableProgramBlock activeBlock;
    int activeBlockIndex;
    int numBlocks;
};

class CompiledProgram {
    ProgramBlock _blocks[maxProgramBlocks];
    int _blockIndexLookup[maxProgramBlocks];

    ProgramStack _state[maxProgramStackFrames];
    ProgramStack* _stateP;

    ProgramBlock* getBlock(InstructionPointer insP, TurnDirection turn);

public:
    CompiledProgram();

    void push();
    void pop();

    // Update the active program block
    void incSteps() { _stateP->activeBlock.numSteps++; }
    void incAmount() { _stateP->activeBlock.amount++; }
    void decAmount() { _stateP->activeBlock.amount--; }
    void setInstruction(bool isDelta);

    bool isDeltaInstruction();
    bool isInstructionSet();
    int getAmount();
    int getNumSteps();

    void finalizeBlock(InstructionPointer endP);

    ProgramBlock* enterBlock(InstructionPointer startP, TurnDirection turnDir);

    void dump();
};


#endif /* CompiledProgram_h */
