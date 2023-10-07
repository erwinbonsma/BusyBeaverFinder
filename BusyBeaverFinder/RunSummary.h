//
//  RunSummary.h
//  BusyBeaverFinder
//
//  Created by Erwin on 07/04/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include <map>
#include <vector>

#include "InterpretedProgram.h"
#include "Utils.h"

class ProgramBlock;

typedef int RunUnitId;

/* Building block for constructing a tree of possible run unit sequences
 */
class RunBlockSequenceNode {
    friend class RunSummaryBase;

    RunUnitId _runUnitId;

    // Run block sequence node indices
    int _childIndex;
    int _siblingIndex;

public:
    RunBlockSequenceNode(RunUnitId runUnitId) : _runUnitId(runUnitId) {}

    RunUnitId getRunUnitId() const { return _runUnitId; }
};

/* Groups a sequence of run units
 */
class RunBlock {
    int _startIndex;  // Index into run unit history
    int _sequenceId;  // Unique Id for this sequence of run units
    int _loopPeriod;  // Loop period (in run units). Zero when sequence is not a loop.

public:
    RunBlock(int startIndex, int sequenceId, int loopPeriod)
    : _startIndex(startIndex), _sequenceId(sequenceId), _loopPeriod(loopPeriod) {}

    bool isLoop() const { return _loopPeriod != 0; }
    int getLoopPeriod() const { return _loopPeriod; }

    // Returns the index into program block stack of the run summary where this run block started.
    int getStartIndex() const { return _startIndex; }

    // Returns a unique identifier for this (type of) run block. Different run blocks have the same
    // identifier when:
    // - their execution sequences match exactly, or
    // - they represent the same loop. In this case, the number of loop iterations may differ
    int getSequenceId() const { return _sequenceId; }

    bool operator==(const RunBlock& other) const {
        return _sequenceId == other._sequenceId;
    }
    bool operator!=(const RunBlock& other) const { return !(*this == other); }
};

/* Summarises a run history constisting of a sequence of run units into higher level run blocks.
 */
class RunSummaryBase {

    // Index to first run unit that is not yet part of a run block
    int _pending;

    // Index into the current loop, if any. It is used to quickly check when loop is broken
    int _loop;

    // The number of run units from the history that has been processed. Note, not all are
    // necessarily part of a run block yet.
    int _processed;

    std::vector<RunBlockSequenceNode> _sequenceBlocks;

    // Cache for areLoopsRotationEqual method.
    // Key: Pair of loop sequence indices (smallest first)
    // Value: Pair of results with first the equality result, and second the offset (if applicable)
    mutable std::map<std::pair<int, int>, std::pair<bool, int>> _rotationEqualityCache;

    // Helper array required by findRepeatedSequence utility function
    // Note: It is not owned by this class (and should therefore not be freed by it)
    int* _helperBuf;

    RunBlockSequenceNode* getChildNode(RunBlockSequenceNode* parent, RunUnitId targetId);

    virtual int getNumRunUnits() const = 0;
    // Returns the identifier for the run unit at the given index in the run history
    virtual int getRunUnitIdAt(int runUnitIndex) const = 0;

    void createRunBlock(int start, int end, int loopPeriod);

    //--------------------------------------------------------------------------------------------

    void dumpRunBlockSequenceNode(int nodeIndex, int level) const;

    int calculateCanonicalLoopIndex(int startIndex, int len) const;
    bool determineRotationEquivalence(int index1, int index2, int len, int &offset) const;

protected:
    // Stack of recently executed run blocks
    std::vector<RunBlock> _runBlocks;

    // Returns true if this resulted in the creation of one or more RunBlocks
    template <class RunUnitHistory>
    bool processNewHistory(const RunUnitHistory& history);

public:
    void setHelperBuffer(int* helperBuf) { _helperBuf = helperBuf; }

    void reset();
    virtual bool processNewRunUnits() = 0;

    bool isInsideLoop() const { return _loop >= 0; }
    int getLoopPeriod() const { return _runBlocks.back().getLoopPeriod(); }
    int getLoopIteration() const;

    // Returns true when the loop just completed an iteration
    bool isAtEndOfLoop() const;

    bool loopContinues(RunUnitId nextId) const { return getRunUnitIdAt(_loop) == nextId; }

    int getNumRunBlocks() const { return (int)_runBlocks.size(); }
    const RunBlock* runBlockAt(int index) const { return &_runBlocks[index]; }
    const RunBlock* getLastRunBlock() const { return &_runBlocks.back(); }

    // Gets the length in run units of a sequence run blocks. The endIndex is exclusive.
    int getRunBlockLength(int startIndex, int endIndex) const;

    // Returns the length in program blocks of the given run block.
    //
    // Note, for loops it counts the repeated executions. Furthermore, the total does not have to
    // be a multiple of the length of the sequence, as the loop may break from anywhere in the
    // sequence.
    //
    // E.g.
    // Seq = A B C (where A, B and C are Program Blocks)
    // => A B C A B C A B X => length = 8, where X is the program block that breaks the loop
    int getRunBlockLength(int index) const { return getRunBlockLength(index, index + 1); }
    int getRunBlockLength(const RunBlock* block) const {
        return getRunBlockLength((int)(block - &_runBlocks[0]));
    }

    // Returns "true" if both loop run blocks are equal when rotations are allowed. E.g. it returns
    // true when comparing "A B C" and "B C A". When this is the case, indexOffset gives the
    // conversion of an index of loop2 to that of loop1: index1 = (index2 + indexOffset) % period
    int areLoopsRotationEqual(const RunBlock* block1, const RunBlock* block2,
                              int &indexOffset) const;

    auto cbegin() const { return _runBlocks.cbegin(); }
    auto cend() const { return _runBlocks.cend(); }

    void dumpSequenceTree() const;
    void dumpCondensed() const;
    void dump() const;
};

class RunSummary : public RunSummaryBase {
    const std::vector<const ProgramBlock *> &_runHistory;

    int getDpDeltaOfProgramBlockSequence(int start, int end) const;

    int getNumRunUnits() const override { return (int)_runHistory.size(); };
    int getRunUnitIdAt(int runUnitIndex) const override {
        return _runHistory[runUnitIndex]->getStartIndex();
    };

public:
    RunSummary(const std::vector<const ProgramBlock *> &runHistory) : _runHistory(runHistory) {}

    // Returns how much DP shifted when executing the sequence of run blocks from firstRunBlock up
    // to lastRunBlock (exclusive).
    int getDpDelta(int firstRunBlock, int lastRunBlock) const;

    const std::vector<RunBlock>& getRunBlocks() const { return _runBlocks; }

    bool processNewRunUnits() override { return processNewHistory(_runHistory); };
};

class MetaRunSummary : public RunSummaryBase {
    const std::vector<RunBlock> &_runHistory;

    int getNumRunUnits() const override { return (int)_runHistory.size(); };
    int getRunUnitIdAt(int runUnitIndex) const override {
        return _runHistory[runUnitIndex].getSequenceId();
    };

public:
    MetaRunSummary(const std::vector<RunBlock> &runHistory) : _runHistory(runHistory) {}

    bool processNewRunUnits() override { return processNewHistory(_runHistory); };
};

template <class RunUnitHistory>
bool RunSummaryBase::processNewHistory(const RunUnitHistory& history) {
//    std::cout << "recordProgramBlock #" << (int)blockIndex << std::endl;

    bool newRunBlocks = false;

    while (_processed < history.size()) {
        if (_loop < 0) {
            int loopPeriod = findRepeatedSequence(&history[_pending], _helperBuf,
                                                  _processed - _pending + 1);
            if (loopPeriod > 0) {                       // Start of new loop
                // std::cout << "Loop detected (period = " << loopPeriod << ")" << std::endl;
                _loop = (int)history.size() - loopPeriod * 2;

                if (_loop != _pending) {
                    createRunBlock(_pending, _loop, 0);
                }
                createRunBlock(_loop, _processed + 1, loopPeriod);
                _pending = -1;
                newRunBlocks = true;
            }
        } else {
            if (history[_loop++] != history.back()) {  // Loop is broken
                // std::cout << "Loop exited!" << std::endl;
                _pending = _processed;
                _loop = -1;
            }
        }
        ++_processed;
    }

    return newRunBlocks;
}
