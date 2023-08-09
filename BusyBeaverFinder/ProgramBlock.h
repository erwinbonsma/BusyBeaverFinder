//
//  ProgramBlock.h
//  BusyBeaverFinder
//
//  Created by Erwin on 25/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef ProgramBlock_h
#define ProgramBlock_h

#include <iostream>
#include <assert.h>

#include "Types.h"

const int maxProgramBlockEntries = 16;

class ProgramBlock {
    int _startIndex;

    // Entries
    ProgramBlock* _entries[maxProgramBlockEntries];
    int _numEntries;

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

    void pushEntry(ProgramBlock* entry) {
        assert(_numEntries < maxProgramBlockEntries);
        _entries[_numEntries++] = entry;
    }
    ProgramBlock* popEntry() { return _entries[--_numEntries]; }

    int numEntryBlocks() const { return _numEntries; }

    const ProgramBlock* entryBlock(int index) const { return _entries[index]; }
    const ProgramBlock* zeroBlock() const { return _zeroBlock; }
    const ProgramBlock* nonZeroBlock() const { return _nonZeroBlock; }

    ProgramBlock* entryBlock(int index) { return _entries[index]; }
    ProgramBlock* zeroBlock() { return _zeroBlock; }
    ProgramBlock* nonZeroBlock() { return _nonZeroBlock; }

    void dump() const;
};

std::ostream &operator<<(std::ostream &os, const ProgramBlock &pb);

#endif /* ProgramBlock_h */
