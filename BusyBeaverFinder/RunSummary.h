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
#include "ProgramBlock.h"
#include "Utils.h"

typedef int RunUnitId;

/* Building block for constructing a tree of possible run unit sequences
 */
class RunBlockSequenceNode {
    friend class RunSummaryBase;

    RunUnitId _runUnitId;

    // Run block sequence node indices
    int _childIndex = 0;
    int _siblingIndex = 0;

    // Index of first run unit that starts the sequence that reaches here
    int _startIndex = 0;

public:
    RunBlockSequenceNode(RunUnitId runUnitId, int startIndex)
    : _runUnitId(runUnitId), _startIndex(startIndex) {}

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

    // Tracks for each sequence when it was last followed by a loop.
    // Key: Sequence ID
    // Value: Index of last run block with this ID that was followed by a loop
    std::map<int, int> _lastOccurenceBeforeLoop;

    // Cache for areLoopsRotationEqual method.
    // Key: Pair of loop sequence indices (smallest first)
    // Value: Pair of results with first the equality result, and second the offset (if applicable)
    mutable std::map<std::pair<int, int>, std::pair<bool, int>> _rotationEqualityCache;

    // Helper array required by findRepeatedSequence utility function
    // Note: It is not owned by this class (and should therefore not be freed by it)
    int* _helperBuf;

    bool _identifyShortLoops {};

    RunBlockSequenceNode* getChildNode(RunBlockSequenceNode* parent, RunUnitId targetId, int start);

    int getNumRunUnits() const { return _processed; }
    // Returns the identifier for the run unit at the given index in the run history
    virtual int getRunUnitIdAt(int runUnitIndex) const = 0;

    int sequenceId(int start, int end);

    void addRunBlock(int start, int sequenceId, int loopPeriod);

    // Creates run blocks for the transition from start to end. By default, creates a single run
    // block. If configured to identify short loops, it creates loop run blocks for loops that ran
    // less than two iterations.
    void createRunBlocks(int start, int end);

    //--------------------------------------------------------------------------------------------

    void dumpRunBlockSequenceNode(int nodeIndex, int level) const;

    int calculateCanonicalLoopIndex(int startIndex, int len) const;
    bool determineRotationEquivalence(int index1, int index2, int len, int &offset) const;

protected:
    // Stack of recently executed run blocks
    std::vector<RunBlock> _runBlocks;

    void resetPending();
    void createRunBlock(int start, int end, int loopPeriod);
    virtual void exitedLoop() {}

    // Returns true if this resulted in the creation of one or more RunBlocks
    template <class RunUnitHistory>
    bool processNewHistory(const RunUnitHistory& history);

public:
    RunSummaryBase(int* helperBuf) : _helperBuf(helperBuf) { reset(); }

    // Do not support copy and assignment to avoid accidental expensive copies. Run summaries
    // should be passed by reference.
    RunSummaryBase(const RunSummaryBase&) = delete;
    RunSummaryBase& operator=(const RunSummaryBase&) = delete;

    int* getHelperBuffer() const { return _helperBuf; }

    void setIdentifyShortLoops(bool flag) { _identifyShortLoops = flag; }

    virtual void reset();
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
    const std::vector<RunBlock>& getRunBlocks() const { return _runBlocks; }

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

    int getStartIndexForSequence(int sequenceId) const {
        return _sequenceBlocks[sequenceId]._startIndex;
    }

    // Returns the index of the last loop with given sequenceId that has at least the given length.
    std::optional<int> findLoopOfLength(int sequenceId, int length) const;

    // Returns "true" if both loop run blocks are equal when rotations are allowed. E.g. it returns
    // true when comparing "A B C" and "B C A". When this is the case, indexOffset gives the
    // conversion of an index of loop2 to that of loop1: index1 = (index2 + indexOffset) % period
    int areLoopsRotationEqual(const RunBlock* block1, const RunBlock* block2,
                              int &indexOffset) const;

    auto cbegin() const { return _runBlocks.cbegin(); }
    auto cend() const { return _runBlocks.cend(); }

    void dumpSequenceTree() const;
    void dumpCondensed(bool hideLegend = false) const;
    void dump() const;
};

typedef std::vector<const ProgramBlock *> RunHistory;

class RunSummary : public RunSummaryBase {
    const RunHistory &_runHistory;

    int getDpDeltaOfProgramBlockSequence(int start, int end) const;

    int getRunUnitIdAt(int runUnitIndex) const override {
        return _runHistory[runUnitIndex]->getStartIndex();
    };

public:
    RunSummary(const RunHistory &runHistory, int* helperBuf)
    : RunSummaryBase(helperBuf), _runHistory(runHistory) {}

    // Do not support copy and assignment to avoid accidental expensive copies. Run summaries
    // should be passed by reference.
    RunSummary(const RunSummary&) = delete;
    RunSummary& operator=(const RunSummary&) = delete;

    // Returns how much DP shifted when executing the sequence of run blocks from firstRunBlock up
    // to lastRunBlock (exclusive).
    int getDpDelta(int firstRunBlock, int lastRunBlock) const;

    bool processNewRunUnits() override { return processNewHistory(_runHistory); };
};

class MetaRunSummary : public RunSummaryBase {
    const std::vector<RunBlock> &_runHistory;

    std::unique_ptr<MetaRunSummary> _metaLoopDetector;
    int _rewriteCount {};

    int getRunUnitIdAt(int runUnitIndex) const override {
        return _runHistory[runUnitIndex].getSequenceId();
    };

protected:
    void exitedLoop() override;

public:
    MetaRunSummary(const std::vector<RunBlock> &runHistory, int* helperBuf)
    : RunSummaryBase(helperBuf), _runHistory(runHistory) {}

    // Do not support copy and assignment to avoid accidental expensive copies. Run summaries
    // should be passed by reference.
    MetaRunSummary(const MetaRunSummary&) = delete;
    MetaRunSummary& operator=(const MetaRunSummary&) = delete;

    void reset() override;

    int rewriteCount() const { return _rewriteCount; }
    bool processNewRunUnits() override { return processNewHistory(_runHistory); };
};

template <class RunUnitHistory>
bool RunSummaryBase::processNewHistory(const RunUnitHistory& history) {
    bool newRunBlocks = false;
    bool didExitLoop = false;

    while (_processed < history.size()) {
        if (_loop < 0) {
            int loopPeriod = findRepeatedSequence(&history[_pending], _helperBuf,
                                                  _processed - _pending + 1);
            if (loopPeriod > 0) {                           // Start of new loop
                _loop = _processed + 1 - loopPeriod * 2;

                createRunBlocks(_pending, _loop);
                createRunBlock(_loop, _processed + 1, loopPeriod);
                _pending = -1;
                newRunBlocks = true;
            }
        } else {
            if (history[_loop++] != history[_processed]) {  // Loop is broken
                _pending = _processed;
                _loop = -1;
                didExitLoop = true;
            }
        }
        ++_processed;
    }

    if (didExitLoop) {
        exitedLoop();
    }

    return newRunBlocks;
}
