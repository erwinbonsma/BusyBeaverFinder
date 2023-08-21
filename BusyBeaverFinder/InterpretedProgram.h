//
//  InterpretedProgram.h
//  BusyBeaverFinder
//
//  Created by Erwin on 25/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//
#pragma once

#include <iostream>

#include "ProgramBlock.h"

class InterpretedProgram {
    void dumpBlock(const ProgramBlock* block, std::ostream &os) const;

public:
    virtual int numProgramBlocks() const = 0;
    virtual const ProgramBlock* programBlockAt(int index) const = 0;
    virtual int indexOf(const ProgramBlock *block) const = 0;

    virtual const ProgramBlock* getEntryBlock() const = 0;

    virtual void dump() const;
};
