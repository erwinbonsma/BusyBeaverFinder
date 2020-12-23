//
//  RunSummary.h
//  BusyBeaverFinder
//
//  Created by Erwin on 07/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef RunSummary_h
#define RunSummary_h

#include <map>

class ProgramBlock;

typedef int ProgramBlockIndex;

// The next to constants should be set such that it never is the limiting factor for hang detection
// (instead, the logic of the available/implemented hang detectors should be)
#ifdef DEBUG
const int maxRunBlockHistoryLength = 32768;
const int maxNumSequenceBlocks = 8192;
#else
const int maxRunBlockHistoryLength = 1000000; // TODO: Make dynamic
const int maxNumSequenceBlocks = 65536;
#endif


class RunBlockSequenceNode {
    friend class RunSummary;

    ProgramBlockIndex _programBlockIndex;

    int _childIndex;
    int _siblingIndex;

    void init(ProgramBlockIndex programBlockIndex);

    ProgramBlockIndex getProgramBlockIndex() const { return _programBlockIndex; }
};

class RunBlock {
    friend class RunSummary;

    int _startIndex;
    int _sequenceIndex;
    int _loopPeriod;

    void init(int startIndex, int sequenceIndex, int loopPeriod);

public:
    bool isLoop() const { return _loopPeriod != 0; }
    int getLoopPeriod() const { return _loopPeriod; }

    // Returns the index into program block stack of the run summary where this run block started.
    int getStartIndex() const { return _startIndex; }

    // Returns a unique identifier for this (type of) run block. Different run blocks have the same
    // index when:
    // - their execution sequences match exactly, or
    // - they represent the same loop. In this case, the number of loop iterations may differ
    int getSequenceIndex() const { return _sequenceIndex; }
};

class RunSummary {
    friend class RunBlock;

    // Stack of recently executed program blocks
    ProgramBlockIndex* _programBlockHistory = nullptr;
    ProgramBlockIndex* _programBlockHistoryMaxP = nullptr;

    // Pointer to current top of the stack
    ProgramBlockIndex* _programBlockHistoryP = nullptr;

    // Pointer to first program block that is not yet part of a run block
    ProgramBlockIndex* _programBlockPendingP = nullptr;

    // Pointer to the previous iteration of the loop when in a loop. Otherwise equals nullptr
    ProgramBlockIndex* _loopP = nullptr;

    // Stack of recently executed run blocks
    RunBlock _runBlockHistory[maxRunBlockHistoryLength];

    RunBlock* _runBlockHistoryMaxP = _runBlockHistory + maxRunBlockHistoryLength;

    // Pointer to threshold where buffer is considered full. As a single invocation of
    // recordProgramBlock can add multiple run blocks, there may still be a few empty spots after
    // the threshold.
    RunBlock* _runBlockHistoryThresholdP = _runBlockHistory + maxRunBlockHistoryLength - 1;

    // Pointer to current top of the stack
    RunBlock* _runBlockHistoryP = nullptr;

    RunBlockSequenceNode _sequenceBlock[maxNumSequenceBlocks];
    int _numSequenceBlocks;

    // Cache for areLoopsRotationEqual method.
    // Key: Pair of loop sequence indices (smallest first)
    // Value: Pair of results with first the equality result, and second the offset (if applicable)
    mutable std::map<std::pair<int, int>, std::pair<bool, int>> _rotationEqualityCache;

    // Helper array required by findRepeatedSequence utility function
    // Note: It is not owned by this class (and should therefore not be freed by it)
    int* _helperBuf;

    void freeDynamicArrays();

    RunBlockSequenceNode* getChildNode(RunBlockSequenceNode* parent, ProgramBlockIndex targetIndex);
    int getSequenceIndex(ProgramBlockIndex* startP, ProgramBlockIndex* endP);

    // Creates a run block for the programs blocks from startP (inclusive) to endP (exclusive)
    void createRunBlock(ProgramBlockIndex* startP, ProgramBlockIndex* endP, int loopPeriod);

    void dumpRunBlockSequenceNode(const RunBlockSequenceNode* node, int level) const;

    int calculateCanonicalLoopIndex(int startIndex, int len) const;
    bool determineRotationEquivalence(int index1, int index2, int len, int &offset) const;

public:
    ~RunSummary();

    void reset();

    // Set the capacity (in program blocks)
    int getCapacity() const;
    void setCapacity(int capacity, int* helperBuf);

    // Returns true if this resulted in the creation of one or more RunBlocks
    bool recordProgramBlock(ProgramBlockIndex blockIndex);

    bool isInsideLoop() const { return _loopP != nullptr; }
    int getLoopPeriod() const { return (int)(_programBlockHistoryP - _loopP); }
    int getLoopIteration() const;

    // Returns true when the loop just completed an iteration
    bool isAtEndOfLoop() const;

    // Returns true when the loop will start a next iteration
    bool isAtStartOfLoop(ProgramBlockIndex nextBlockIndex) const {
        return isAtEndOfLoop() && *_loopP == nextBlockIndex;
    }

    bool loopContinues(ProgramBlockIndex nextBlockIndex) const {
        return *_loopP == nextBlockIndex;
    }

    bool hasSpaceRemaining() const { return _runBlockHistoryP < _runBlockHistoryThresholdP; }

    int getNumProgramBlocks() const { return (int)(_programBlockHistoryP - _programBlockHistory); }
    ProgramBlockIndex programBlockIndexAt(int index) const { return _programBlockHistory[index]; }
    ProgramBlockIndex getLastProgramBlockIndex() const { return *(_programBlockHistoryP - 1); }

    int getNumRunBlocks() const { return (int)(_runBlockHistoryP - _runBlockHistory); }
    const RunBlock* runBlockAt(int index) const { return _runBlockHistory + index; }
    const RunBlock* getLastRunBlock() const { return (_runBlockHistoryP - 1); }

    // Gets the length in program blocks of a sequence of one or more run blocks. The endIndex is
    // exclusive
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
        return getRunBlockLength((int)(block - _runBlockHistory));
    }

    // Returns "true" if both loop run blocks are equal when rotations are allowed. E.g. it returns
    // true when comparing "A B C" and "B C A". When this is the case, indexOffset gives the
    // conversion of an index of loop2 to that of loop1: index1 = (index2 + indexOffset) % period
    int areLoopsRotationEqual(const RunBlock* block1, const RunBlock* block2,
                              int &indexOffset) const;

    void dumpSequenceTree() const;
    void dumpCondensed() const;
    void dump() const;
};

#endif /* RunSummary_h */
