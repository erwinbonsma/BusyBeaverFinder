//
//  InterpretedProgram.h
//  BusyBeaverFinder
//
//  Created by Erwin on 25/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//
#pragma once

#include <iostream>
#include <sstream>
#include <deque>
#include <vector>

#include "ProgramBlock.h"

class InterpretedProgram {
    void dumpBlock(const ProgramBlock* block, std::ostream &os) const;

protected:
    char charForIndex(int index) const;
    int indexForChar(char ch) const;

public:
    virtual int numProgramBlocks() const = 0;
    virtual const ProgramBlock* programBlockAt(int index) const = 0;
    virtual int indexOf(const ProgramBlock *block) const {
        return (int)(block - getEntryBlock());
    };

    const ProgramBlock* getEntryBlock() const { return programBlockAt(0); };

    void dump() const;

    void dumpShortProgram(std::ostream &os) const;
    void dumpBlockSizes(std::ostream &os) const;
    std::string shortProgramString() const;
    std::string blockSizeString() const;
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
};

class InterpretedProgramFromString : public InterpretedProgram {
    std::vector<ProgramBlock> _blocks;

    std::deque<int> sizesFromString(std::string& sizes);
    ProgramBlock* blockForChar(char ch);
    void finalizeBlock(std::string& blockSpec, int numSteps);
public:
    InterpretedProgramFromString(std::string& program, std::string& sizes);

    int numProgramBlocks() const override { return static_cast<int>(_blocks.size()); }
    const ProgramBlock* programBlockAt(int index) const override { return &_blocks[index]; }
};
