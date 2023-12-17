//
//  ProgramBlock.h
//  BusyBeaverFinder
//
//  Created by Erwin on 25/03/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include <iostream>
#include <vector>

#include "Types.h"

class ProgramBlock {
    int _startIndex;

    // Entries
    std::vector<ProgramBlock*> _entries;

    // Exits
    ProgramBlock* _zeroBlock;
    ProgramBlock* _nonZeroBlock;

    bool _isFinalized;
    bool _isDelta;
    int _instructionAmount;
    int _numSteps;

public:
    ProgramBlock();

    void init(int startIndex);
    void reset();

    void finalizeHang();
    void finalizeExit(int numSteps);
    void finalize(bool isDelta, int amount, int numSteps,
                  ProgramBlock* zeroBlock, ProgramBlock* nonZeroBlock);

    // Index that uniquely specifies the starting position (including turn direction)
    int getStartIndex() const { return _startIndex; }

    bool isFinalized() const { return _isFinalized; }

    bool isExit() const { return _instructionAmount == 0 && _numSteps >= 0; }
    bool isHang() const { return _numSteps < 0; }
    bool isDelta() const { return _isDelta; }
    int getInstructionAmount() const { return _instructionAmount; }
    int getNumSteps() const { return _numSteps; }

    void pushEntry(ProgramBlock* entry) { _entries.push_back(entry); }
    ProgramBlock* popEntry() { auto entry = _entries.back(); _entries.pop_back(); return entry; }

    int numEntryBlocks() const { return static_cast<int>(_entries.size()); }

    const ProgramBlock* entryBlock(int index) const { return _entries[index]; }
    const ProgramBlock* zeroBlock() const { return _zeroBlock; }
    const ProgramBlock* nonZeroBlock() const { return _nonZeroBlock; }

    ProgramBlock* entryBlock(int index) { return _entries[index]; }
    ProgramBlock* zeroBlock() { return _zeroBlock; }
    ProgramBlock* nonZeroBlock() { return _nonZeroBlock; }

    void dump() const;
};

std::ostream &operator<<(std::ostream &os, const ProgramBlock &pb);

