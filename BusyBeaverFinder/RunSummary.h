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

typedef size_t RunUnitId;

/* Building block for constructing a tree of possible run unit sequences
 */
class RunBlockSequenceNode {
    friend class RunSummaryBase;

    RunUnitId _runUnitId;

    // Run block sequence node indices
    size_t _childIndex = 0;
    size_t _siblingIndex = 0;

    // Index of first run unit that starts the sequence that reaches here
    size_t _startIndex = 0;

public:
    RunBlockSequenceNode(RunUnitId runUnitId, size_t startIndex)
    : _runUnitId(runUnitId), _startIndex(startIndex) {}

    RunUnitId getRunUnitId() const { return _runUnitId; }
};

/* Groups a sequence of run units
 *
 * Run blocks are the same if their sequences match. For loops, the number of executed iterations
 * may differ.
 */
class RunBlock {
    size_t _startIndex;  // Index into run unit history
    size_t _sequenceId;  // Unique Id for this sequence of run units
    size_t _loopPeriod;  // Loop period (in run units). Zero when sequence is not a loop.

public:
    RunBlock(size_t startIndex, size_t sequenceId, size_t loopPeriod)
    : _startIndex(startIndex), _sequenceId(sequenceId), _loopPeriod(loopPeriod) {}

    bool isLoop() const { return _loopPeriod != 0; }
    size_t getLoopPeriod() const { return _loopPeriod; }

    // Returns the index into program block stack of the run summary where this run block started.
    size_t getStartIndex() const { return _startIndex; }

    // Returns a unique identifier for this (type of) run block. Different run blocks have the same
    // identifier when:
    // - their execution sequences match exactly, or
    // - they represent the same loop. In this case, the number of loop iterations may differ
    size_t getSequenceId() const { return _sequenceId; }

    bool operator==(const RunBlock& other) const {
        return _sequenceId == other._sequenceId;
    }
    bool operator!=(const RunBlock& other) const { return !(*this == other); }
};

/* Summarises a run history constisting of a sequence of run units into higher level run blocks.
 */
class RunSummaryBase {
    // Index to first run unit that is not yet part of a run block
    size_t _pending;

    // Index into the current loop, if any. It is used to quickly check when loop is broken
    size_t _loop;

    // The number of run units from the history that has been processed. Note, not all are
    // necessarily part of a run block yet.
    size_t _processed;

    bool _inLoop;

    std::vector<RunBlockSequenceNode> _sequenceBlocks;

    // Tracks for each sequence when it was last followed by a loop.
    // Key: Sequence ID
    // Value: Index of last run block with this ID that was followed by a loop
    std::map<size_t, size_t> _lastOccurenceBeforeLoop;

    // Cache for areLoopsRotationEqual method.
    // Key: Pair of loop sequence indices (smallest first)
    // Value: Pair of results with first the equality result, and second the offset (if applicable)
    mutable std::map<std::pair<int, int>, std::pair<bool, int>> _rotationEqualityCache;

    // Helper array required by findRepeatedSequence utility function
    // Note: It is not owned by this class (and should therefore not be freed by it)
    int* _helperBuf;

    bool _identifyShortLoops {};

    RunBlockSequenceNode*
    getChildNode(RunBlockSequenceNode* parent, RunUnitId targetId, size_t start);

    size_t getNumRunUnits() const { return _processed; }

    // Returns the identifier for the run unit at the given index in the run history
    // TODO: Can this be made non-virtual?
    virtual size_t getRunUnitIdAt(size_t runUnitIndex) const = 0;

    size_t sequenceId(size_t start, size_t end);

    void addRunBlock(size_t start, size_t sequenceId, size_t loopPeriod);

    // Creates run blocks for the transition from start to end. By default, creates a single run
    // block. If configured to identify short loops, it creates loop run blocks for loops that ran
    // less than two iterations.
    void createRunBlocks(size_t start, size_t end);

    //--------------------------------------------------------------------------------------------

    void dumpRunBlockSequenceNode(size_t nodeIndex, size_t level) const;

    // Returns the index of the canonical loop start relative to the startIndex.
    size_t calculateCanonicalLoopIndex(size_t startIndex, size_t len) const;
    bool determineRotationEquivalence(size_t idx1, size_t idx2, size_t len, size_t &offset) const;

protected:
    // Stack of recently executed run blocks
    std::vector<RunBlock> _runBlocks;

    void resetPending();
    void createRunBlock(size_t start, size_t end, size_t loopPeriod);
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

    // Enables short-loop detection
    //
    // It enables detection of loops that run fewer than two full iterations. It does so by
    // considering the run block that precedes the potential short loop. When the run units that
    // follow this run block match the run blocks of the last loop (if any) that followed this run
    // block.
    //
    // For example consider ABCDDDDEFGGGABCDEFGGG.
    // Without short-loop detection it is split into run blocks as follows:
    //   ABC [D]DDD EF [G]GG ABCDEF [G]GG
    // With short-loop detection it is instead split as:
    //   ABC [D]DDD EF [G]GG ABC [D] EF [G]GG
    // Here D is considered as a single-iteration loop, as previously the transition of ABC was
    // followed by a longer-running instance of this loop.
    //
    // Notable limitations:
    // - It does not work (reliably) when a given run block can be followed by multiple loops.
    //   It only recognizes the last loop.
    // - It does not work when the short-loop becomes part of a bigger loop.
    //   Example: ABCDDDABCDABCDDD
    //            becomes    ABC [D]DD [ABCD]ABCD [D]D
    //            instead of ABC [D]DD ABC [D] ABC [D]DD
    void setIdentifyShortLoops(bool flag) { _identifyShortLoops = flag; }

    virtual void reset();
    virtual bool processNewRunUnits() = 0;

    bool isInsideLoop() const { return _inLoop; }
    size_t getLoopPeriod() const { return _runBlocks.back().getLoopPeriod(); }
    size_t getLoopIteration() const;

    // Returns true when the loop just completed an iteration
    bool isAtEndOfLoop() const;

    bool loopContinues(RunUnitId nextId) const { return getRunUnitIdAt(_loop) == nextId; }

    size_t getNumRunBlocks() const { return _runBlocks.size(); }
    const RunBlock* runBlockAt(size_t index) const { return &_runBlocks[index]; }
    const RunBlock* getLastRunBlock() const { return &_runBlocks.back(); }
    const std::vector<RunBlock>& getRunBlocks() const { return _runBlocks; }

    // Gets the length in run units of a sequence run blocks. The endIndex is exclusive.
    size_t getRunBlockLength(size_t startIndex, size_t endIndex) const;

    // Returns the length in program blocks of the given run block.
    //
    // Note, for loops it counts the repeated executions. Furthermore, the total does not have to
    // be a multiple of the length of the sequence, as the loop may break from anywhere in the
    // sequence.
    //
    // E.g.
    // Seq = A B C (where A, B and C are Program Blocks)
    // => A B C A B C A B X => length = 8, where X is the program block that breaks the loop
    size_t getRunBlockLength(size_t index) const { return getRunBlockLength(index, index + 1); }
    size_t getRunBlockLength(const RunBlock* block) const {
        return getRunBlockLength((int)(block - &_runBlocks[0]));
    }

    size_t getStartIndexForSequence(size_t sequenceId) const {
        return _sequenceBlocks[sequenceId]._startIndex;
    }

    // Returns the index of the last loop with given sequenceId that has at least the given length.
    std::optional<size_t> findLoopOfLength(size_t sequenceId, size_t length) const;

    // Returns "true" if both loop run blocks are equal when rotations are allowed. E.g. it returns
    // true when comparing "A B C" and "B C A". When this is the case, indexOffset gives the
    // conversion of an index of loop2 to that of loop1: index1 = (index2 + indexOffset) % period
    bool areLoopsRotationEqual(const RunBlock* block1, const RunBlock* block2,
                               size_t &indexOffset) const;

    auto cbegin() const { return _runBlocks.cbegin(); }
    auto cend() const { return _runBlocks.cend(); }

    void dumpSequenceTree() const;
    void dumpCondensed(bool hideLegend = false) const;
    void dump() const;
};

typedef std::vector<const ProgramBlock *> RunHistory;

class RunSummary : public RunSummaryBase {
    const RunHistory &_runHistory;

    size_t getDpDeltaOfProgramBlockSequence(size_t start, size_t end) const;

    size_t getRunUnitIdAt(size_t runUnitIndex) const override {
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
    size_t getDpDelta(size_t firstRunBlock, size_t lastRunBlock) const;

    bool processNewRunUnits() override { return processNewHistory(_runHistory); };
};

class MetaRunSummary : public RunSummaryBase {
    const std::vector<RunBlock> &_runHistory;

    std::unique_ptr<MetaRunSummary> _metaLoopDetector;
    size_t _metaLoopIterations;
    size_t _rewriteCount {};

    RunUnitId getRunUnitIdAt(size_t runUnitIndex) const override {
        return _runHistory[runUnitIndex].getSequenceId();
    };

    // Checks if it is possible to replace already collapsed run blocks into a larger run block
    // that represents an overarching loop. If so, it rewrites history and increases the rewrite
    // count.
    //
    // This will for example convert this run summary:
    //   #22*44.1 #23 #10*46.0 #12 #22*46.1 #23 #10*51.0 #12 #22*1.0 #27*4.0
    //   #22*46.1 #23 #10*48.0 #12 #22*48.1 #23 #10*50.0 #12 #22*1.0 #27*1.0
    //
    // Into this meta-run summary:
    //   #32*2
    // with:
    //   32 = 22 23 10 12 22 23 10 12 22 27
    //
    // Intead of:
    //   #19*2.1 #20 #19*2.1 #20
    // with:
    //   19 = 22 23 10 12
    //   20 = 27
    void attemptLoopCollapse();

    void checkIsFullyPeriodic();

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

    size_t rewriteCount() const { return _rewriteCount; }
    bool processNewRunUnits() override { return processNewHistory(_runHistory); }
};

template <class RunUnitHistory>
bool RunSummaryBase::processNewHistory(const RunUnitHistory& history) {
    bool newRunBlocks = false;
    bool didExitLoop = false;

    while (_processed < history.size()) {
        if (_inLoop) {
            if (history[_loop++] != history[_processed]) {  // Loop is broken
                _pending = _processed;
                _inLoop = false;
                didExitLoop = true;
            }
        } else {
            size_t loopPeriod = findRepeatedSequence(&history[_pending], _helperBuf,
                                                     _processed - _pending + 1);
            if (loopPeriod > 0) {                           // Start of new loop
                _loop = _processed + 1 - loopPeriod * 2;

                createRunBlocks(_pending, _loop);
                createRunBlock(_loop, _processed + 1, loopPeriod);
                _inLoop = true;
                newRunBlocks = true;
            }
        }
        ++_processed;
    }

    if (didExitLoop) {
        exitedLoop();
    }

    return newRunBlocks;
}
