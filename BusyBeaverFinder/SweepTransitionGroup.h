//
//  SweepTransitionGroup.h
//  BusyBeaverFinder
//
//  Created by Erwin on 13/11/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//
#pragma once

#include <map>
#include <set>

#include "LoopAnalysis.h"
#include "Utils.h"
#include "RunSummary.h"

class Data;
class ExecutionState;

/* Different types of behaviour at one end of the sweep. These are determined during analysis and
 * require different checks to proof that the program is indeed hanging.
 */
enum class SweepEndType : int {
    UNKNOWN = 0,

    // TODO: Rename to MONOTONOUS_GROWTH
    /* The sequence is growing each sweep. Each iteration it adds one (or more) non-exit values
     * to the sweep body.
     */
    STEADY_GROWTH = 1,

    // TODO: Rename to NONUNIFORM_GROWTH
    /* The sequence is growing, but not each sweep. This end value of the sequence can take two
     * (or more) different exit values. For at least one value, the sweep will change it into
     * another exit value. For at least one other value, the sweep will change it in a non-exit
     * value (i.e. it will extend the sweep body).
     */
    IRREGULAR_GROWTH,

    /* The sweep ends at a fixed position, with a constant value.
     *
     * Note: Value changes during a transition are ignored. Only the final value matters.
     */
    FIXED_POINT_CONSTANT_VALUE,

    /* The sweep ends at a fixed position, which can take multiple but a fixed number of values.
     */
    FIXED_POINT_MULTIPLE_VALUES,

    /* The sweep ends at a fixed position with an increasing value.
     */
    FIXED_POINT_INCREASING_VALUE,

    /* The sweep ends at a fixed position with a decreasing value.
     */
    FIXED_POINT_DECREASING_VALUE,

    /* The sweep ends in an "appendix" sequence. This appendix consists of two (or more)
     * different exit values. These values can change each sweep, impacting where the sweep ends
     * the next time. The appendix grows over time but this is aperiodic. Some kind of binary
     * counting is realized, and the size of appendix grows logarithmitically. The side of the
     * appendix that is attached to the body of the sweep has a fixed position.
     */
    FIXED_APERIODIC_APPENDIX,

    /* The sweep end type is unsupported (or to complex to be recognized by the current logic).
     */
    UNSUPPORTED
};

std::ostream &operator<<(std::ostream &os, SweepEndType sweepEndType);

enum class SweepValueChangeType : int {
    // The sweep loop does not change values
    NO_CHANGE,

    // Each value is changed by the same amount
    UNIFORM_CHANGE,

    // There are multiple changes, of different amounts, but all with the same sign
    MULTIPLE_ALIGNED_CHANGES,

    // There are multiple changes, of different amounts, and with different signs
    MULTIPLE_OPPOSING_CHANGES,
};

class SweepLoopAnalysis : public LoopAnalysis {
    friend std::ostream &operator<<(std::ostream&, const SweepLoopAnalysis&);

    const RunBlock* _loopRunBlock;
    std::vector<const ProgramBlock *> _programBlocks;

    SweepValueChangeType _sweepValueChangeType;

    // Representative sweep value change:
    // - NO_CHANGE: Value is zero
    // - UNIFORM_CHANGE: All values in the sequence are changed by this amount
    // - MULTIPLE_ALIGNED_CHANGES: This is one of the changes. Other changes have the same sign
    // - MULTIPLE_OPPOSING_CHANGES: This is one of the changes (but its value is not useful for
    //   further analysis)
    int _sweepValueChange;
    std::set<int> _sweepValueChanges;

    // Map from exit value to the instruction in the loop that can cause this exit. Only anytime
    // exits are considered. Nevertheless, there may be more than one possible exit for a given
    // value. This is the case when the loop moves more than one cell each iteration, i.e.
    // abs(dataPointerDelta()) > 1.
    std::multimap<int, int> _exitMap;

    bool _requiresFixedInput;
    int _requiredInput;

protected:
    bool finishAnalysis() override;

public:
    const RunBlock* loopRunBlock() const { return _loopRunBlock; }
    const bool movesRightwards() const { return dataPointerDelta() > 0; }

    SweepValueChangeType sweepValueChangeType() const { return _sweepValueChangeType; }

    int sweepValueChange() const { return _sweepValueChange; }
    auto sweepValueChanges() const { return makeProxyIterator(_sweepValueChanges); }

    bool isExitValue(int value) const;
    int numberOfExitsForValue(int value) const;
    void collectInsweepDeltasAfterExit(int exitInstruction, DataDeltas &dataDeltas) const;

    // Returns iterator over exit values
    auto exitValues() const { return makeKeyIterator(_exitMap); }

    bool canSweepChangeValueTowardsZero(int value) const;

    bool requiresFixedInput() const { return _requiresFixedInput; }

    bool analyzeSweepLoop(const RunBlock* runBlock, const ExecutionState& execution);
};

std::ostream &operator<<(std::ostream &os, const SweepLoopAnalysis& sta);

class SweepTransitionAnalysis : public SequenceAnalysis {
    int _pbStartIndex;

public:
    // The indexes are run block indices. The end index is exclusive.
    void analyzeSweepTransition(int startIndex, int endIndex, const ExecutionState& execution);

    bool transitionEquals(int startIndex, int endIndex, const ExecutionState& execution) const;

    void dump() const;
};

std::ostream &operator<<(std::ostream &os, const SweepTransitionAnalysis& sta);

struct SweepTransition {
    const SweepTransitionAnalysis *transition;
    int nextLoopStartIndex;
    mutable int numOccurences;

    SweepTransition() : transition(nullptr), nextLoopStartIndex(0), numOccurences(0) {}
    SweepTransition(const SweepTransitionAnalysis *sta, int nextLoopStartIndex)
    : transition(sta), nextLoopStartIndex(nextLoopStartIndex), numOccurences(1) {}
};

class SweepTransitionGroup;
class SweepEndTypeAnalysis {
protected:
    const SweepTransitionGroup& _group;

    int deltaAfterExit(const LoopExit& loopExit, const SweepTransition& trans) const;

    void addInsweepDeltasAfterTransition(SweepTransition st, DataDeltas &dataDeltas) const;

    void addInSweepDeltasForExit(std::set<int> &deltas, int exitInstruction) const;

    // Determines all in-sweep deltas that can occur as part of a loop exit and adds them to the
    // given set. This method considers the following changes:
    // 1. Changes by the incoming loop. Due to the exit, these deltas can differ from the steady
    //    state deltas applied during the sweep.
    // 2. Changes by the subsequent transition(s). These are applied on top of the deltas of the
    //    incoming loop.
    // 3. Bootstrap only changes by the outgoing loop.
    void collectExitRelatedInsweepDeltas(std::set<int> &deltas) const;

    void collectSweepDeltas(std::set<int> &deltas) const;

    bool valueCanBeChangedToExitByDeltas(int value, std::set<int> &deltas) const;

public:
    SweepEndTypeAnalysis(const SweepTransitionGroup& group) : _group(group) {}

    virtual SweepEndType determineSweepEndType() = 0;
};

class SweepEndTypeAnalysisZeroExits : SweepEndTypeAnalysis {
    // Counts transitions where an exit value remains unchanged, or changes into another exit.
    int _exitToExit;

    // Counts transitions where an exit value is converted to a value that can never be converted
    // into an exit. It considers sweep changes as well as any in-sweep deltas by transitions.
    int _exitToSweepBody;

    // Counts transitions where an exit value is converted to a value that is not an exit, but might
    // be changed into one later. It might also be changed to a definite sweep body value, or always
    // remain in limbo.
    int _exitToLimbo;

    int _limboToExitBySweep;
    int _limboToExitByLoopExit;
    int _limboToSweepBody;

    bool _exitValueChanges;

    bool analyseExits();
    SweepEndType classifySweepEndType();

public:
    SweepEndTypeAnalysisZeroExits(const SweepTransitionGroup& group)
    : SweepEndTypeAnalysis(group) {};

    SweepEndType determineSweepEndType() override;
};

class SweepEndTypeAnalysisNonZeroExits : SweepEndTypeAnalysis {
public:
    SweepEndTypeAnalysisNonZeroExits(const SweepTransitionGroup& group)
    : SweepEndTypeAnalysis(group) {};

    SweepEndType determineSweepEndType() override;
};

class SweepTransitionGroup {
    friend std::ostream &operator<<(std::ostream&, const SweepTransitionGroup&);
    friend SweepEndTypeAnalysis;
    friend SweepEndTypeAnalysisZeroExits;
    friend SweepEndTypeAnalysisNonZeroExits;

    // The loop that start this (group of) transition(s).
    const SweepLoopAnalysis *_incomingLoop;
    const SweepLoopAnalysis *_outgoingLoop;

    // The mid-sweep transition of the outgoing sweep, if any.
    const SweepTransitionAnalysis *_midSweepTransition;

    // Map from a given loop exit to the transition(s) that follows it.
    //
    // Note: Most exits are followed by either no or one transition. However, it is possible that
    // there is more than one, which can happen if the transition depends on nearby data values.
    std::multimap<int, SweepTransition> _transitionMap;

    SweepEndTypeAnalysisZeroExits _zeroExitEndTypeAnalysis;
    SweepEndTypeAnalysisNonZeroExits _nonZeroExitEndTypeAnalysis;

    SweepEndType _sweepEndType;
    bool _locatedAtRight;

    // Combined change made both by incoming and outgoing loop
    SweepValueChangeType _sweepValueChangeType;
    int _sweepValueChange;

    DataDeltas _outsideDeltas;

    // The direction of changes inside the sweep made by the transitions. It is only set when the
    // sweep loops themselves do not make any combined change (as otherwise the latter is leading).
    int _insideSweepTransitionDeltaSign;

    bool determineCombinedSweepValueChange();

protected:
    // The deltas of the sweep end-point after each sweep. Positive deltas means that the sequence
    // grows. The sum of all deltas can only be zero (fixed point) or positive (growing sequence).
    // However, individual deltas can be negative.
    std::vector<int> _sweepExitDeltas;

    // Indicates if the hang is locked into a periodic behavior at the meta-run level. Not only
    // should the sequence of meta run-blocks be periodic, also the length of the run-block loops
    // that do not have a fixed length (i.e. the sweep loops) should increase with a fixed number
    // of iterations each period. When this is the case, the checks on the changes made by the
    // loops and transition sequences associated with this transition group can be a bit more
    // lenient.
    virtual bool hangIsMetaPeriodic() { return false; }

    void setEndType(SweepEndType endType) {
        assert(!didDetermineEndType());
        _sweepEndType = endType;
    }

    int numberOfExitsForValue(int value) const;
    int numberOfTransitionsForExitValue(int value) const;

    virtual bool determineZeroExitSweepEndType();
    virtual bool determineNonZeroExitSweepEndType();
    virtual bool determineSweepEndType();

    virtual bool onlyZeroesAhead(DataPointer dp, const Data& data) const;

public:
    SweepTransitionGroup();

    void addExitDelta(int delta) { _sweepExitDeltas.push_back(delta); }

    bool locatedAtRight() const { return _locatedAtRight; }
    SweepEndType endType() const { return _sweepEndType; }
    bool didDetermineEndType() { return (_sweepEndType != SweepEndType::UNKNOWN &&
                                         _sweepEndType != SweepEndType::UNSUPPORTED); }

    const DataDeltas& outsideDeltas() const { return _outsideDeltas; }
    int insideSweepTransitionDeltaSign() const { return _insideSweepTransitionDeltaSign; }

    bool isSweepGrowing() const;
    bool isSweepGrowthConstant() const;

    const SweepLoopAnalysis* incomingLoop() const { return _incomingLoop; }
    const SweepLoopAnalysis* outgoingLoop() const { return _outgoingLoop; }
    void setIncomingLoop(const SweepLoopAnalysis* loop) { _incomingLoop = loop; }
    void setOutgoingLoop(const SweepLoopAnalysis* loop) { _outgoingLoop = loop; }

    const SweepTransitionAnalysis* midSweepTransition() const { return _midSweepTransition; }
    void setMidSweepTransition(const SweepTransitionAnalysis* sta) { _midSweepTransition = sta; }

    // The combined change to the sequence made by both the sweep-loops
    SweepValueChangeType combinedSweepValueChangeType() const { return _sweepValueChangeType; }
    int combinedSweepValueChange() const { return _sweepValueChange; }

    bool canSweepChangeValueTowardsZero(int value) const;

    bool hasTransitionForExit(int exitIndex) const {
        return _transitionMap.find(exitIndex) != _transitionMap.end();
    }
    const SweepTransition* addTransitionForExit(int exitIndex, SweepTransition st) {
        return &(_transitionMap.insert({exitIndex, st})->second);
    }
    const SweepTransition* findTransitionMatching(int exitInstruction,
                                                  int transitionStartIndex,
                                                  int transitionEndIndex,
                                                  const ExecutionState& execution) const;
    bool hasUniqueTransitions() const;

    virtual void clear();
    bool analyzeSweeps();
    bool analyzeGroup();

    bool allOutsideDeltasMoveAwayFromZero(DataPointer dp, const Data& data) const;
    Trilian proofHang(DataPointer dp, const Data& data);

    std::ostream& dumpExitDeltas(std::ostream &os) const;
    virtual std::ostream& dump(std::ostream &os) const;
};

std::ostream &operator<<(std::ostream &os, const SweepTransitionGroup &group);
