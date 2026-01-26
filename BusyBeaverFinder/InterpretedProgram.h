//
//  InterpretedProgram.h
//  BusyBeaverFinder
//
//  Created by Erwin on 25/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//
#pragma once

#include <span>
#include <iostream>

#include "ProgramBlock.h"

class InterpretedProgram {
    void dumpBlock(const ProgramBlock* block, std::ostream &os) const;

public:
    virtual int numProgramBlocks() const = 0;
    virtual const ProgramBlock* programBlockAt(int index) const = 0;
    virtual int indexOf(const ProgramBlock *block) const {
        return (int)(block - getEntryBlock());
    };

    virtual const ProgramBlock* getEntryBlock() const = 0;

    virtual void dump() const;
};

// TODO: Switch to std::span once my XCode supports this
class InterpretedProgramFromArray : public InterpretedProgram {
    const ProgramBlock* _blocks;
    int _numBlocks;

public:
    InterpretedProgramFromArray(const ProgramBlock* blocks, int numBlocks)
    : _blocks(blocks), _numBlocks(numBlocks) {}

    int numProgramBlocks() const override { return _numBlocks; }
    const ProgramBlock* programBlockAt(int index) const override { return _blocks + index; }
    const ProgramBlock* getEntryBlock() const override { return _blocks; }
};
