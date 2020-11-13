//
//  SweepTransitionGroup.h
//  BusyBeaverFinder
//
//  Created by Erwin on 13/11/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef SweepTransitionGroup_h
#define SweepTransitionGroup_h

#include "LoopAnalysis.h"
#include "Utils.h"
#include "RunSummary.h"

class Data;
class ProgramExecutor;

#include <map>
#include <set>

/* Different types of behaviour at one end of the sweep. These are determined during analysis and
 * require different checks to proof that the program is indeed hanging.
 */
enum class SweepEndType : int {
    UNKNOWN = 0,

    /* The sequence is growing each sweep. Each iteration it adds one (or more) non-exit values
     * to the sweep body.
     */
    STEADY_GROWTH = 1,

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

    /* The sweep always ends in an "appendix" sequence. This appendix consists of two (or more)
     * different exit values. These values can change each sweep, impacting where the sweep ends
     * the next time. The appendix can also grow over time. However, the side of the appendix that
     * is attached to the body of the sweep has a fixed position.
     */
    FIXED_GROWING
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

    bool hasIndirectExitsForValueAfterExit(int value, int exitInstruction) const;

public:
    const RunBlock* loopRunBlock() const { return _loopRunBlock; }

    SweepValueChangeType sweepValueChangeType() const { return _sweepValueChangeType; }

    int sweepValueChange() const { return _sweepValueChange; }
    auto sweepValueChanges() const { return makeProxyIterator(_sweepValueChanges); }

    bool isExitValue(int value) const;
    int numberOfExitsForValue(int value) const;
    bool hasIndirectExitsForValue(int value) const;

    // Returns iterator over exit values
    auto exitValues() const { return makeKeyIterator(_exitMap); }

    bool canSweepChangeValueTowardsZero(int value) const;

    bool requiresFixedInput() const { return _requiresFixedInput; }

    bool analyzeSweepLoop(const RunBlock* runBlock, const ProgramExecutor& executor);
};

std::ostream &operator<<(std::ostream &os, const SweepLoopAnalysis& sta);

class SweepTransitionAnalysis : public SequenceAnalysis {
public:
    // The indexes are run block indices. The end index is exclusive.
    bool analyzeSweepTransition(int startIndex, int endIndex, const ProgramExecutor& executor);

    bool transitionEquals(int startIndex, int endIndex, const ProgramExecutor& executor) const;

    void dump() const;
};

std::ostream &operator<<(std::ostream &os, const SweepTransitionAnalysis& sta);

struct SweepLoopExit {
    const SweepLoopAnalysis *loop;
    int exitInstructionIndex;

    SweepLoopExit(const SweepLoopAnalysis *sla, int index)
    : loop(sla), exitInstructionIndex(index) {}

    const LoopExit& loopExit() const { return loop->exit(exitInstructionIndex); }
    bool exitsOnZero() const;

    bool operator==(const SweepLoopExit& rhs) {
        return loop == rhs.loop && exitInstructionIndex == rhs.exitInstructionIndex;
    }
    friend bool operator<(const SweepLoopExit& lhs, const SweepLoopExit& rhs) {
        return (std::tie(lhs.loop, lhs.exitInstructionIndex) <
                std::tie(rhs.loop, rhs.exitInstructionIndex));
    }
};

namespace std {
template <> struct hash<SweepLoopExit> {
    typedef size_t result_type;
    typedef SweepLoopExit argument_type;
    size_t operator()(const SweepLoopExit& sle) const;
};
}

struct SweepTransition {
    const SweepTransitionAnalysis *transition;
    const SweepLoopAnalysis *nextLoop;

    SweepTransition() : transition(nullptr), nextLoop(nullptr) {}
    SweepTransition(const SweepTransitionAnalysis *sta, const SweepLoopAnalysis *sla)
    : transition(sta), nextLoop(sla) {}
};

class SweepHangDetector;
class SweepTransitionGroup {
    friend std::ostream &operator<<(std::ostream&, const SweepTransitionGroup&);

    const SweepHangDetector *_parent;
    const SweepTransitionGroup *_sibling;

    // The loops that can start this transition. This is a vector so that we can distinguish
    // different versions of the same loop, only differing by the entry instruction. The key is
    // the loop's sequence index.
    std::map<int, const SweepLoopAnalysis*> _loops;

    // Map from a given loop exit to transition that follows it.
    std::map<SweepLoopExit, SweepTransition> _transitions;

    SweepEndType _sweepEndType;
    bool _locatedAtRight;

    DataDeltas _outsideDeltas;

    // The direction of changes inside the sweep made by the transitions. It is only set when the
    // sweep loops themselves do not make any combined change (as otherwise the latter is leading).
    int _insideSweepTransitionDeltaSign;

    int numberOfTransitionsForExitValue(int value) const;
    int numberOfExitsForValue(int value) const;

    // Checks if the given value can be modified by the next sweep loop to result in an exit.
    // Here value is the final value of the data cell that caused the loop exit, and dpOffset gives
    // the offset wrt to this value where the next loop starts.
    bool hasIndirectExitsForValue(int value, int dpOffset) const;

    bool determineSweepEndType();

public:
    void init(const SweepHangDetector *parent, const SweepTransitionGroup *sibling);

    bool locatedAtRight() const { return _locatedAtRight; }
    SweepEndType endType() const { return _sweepEndType; }

    const DataDeltas& outsideDeltas() const { return _outsideDeltas; }
    int insideSweepTransitionDeltaSign() const { return _insideSweepTransitionDeltaSign; }

    // Returns a loop analysis representing the exiting loops for this transition group. Due to
    // rotational-equivalence, there can be multiple loops analysis. An arbitrary analysis is
    // returned. This analysis should only be used to inspect values that are the same for all
    // analysis.
    const SweepLoopAnalysis* loop() const { return _loops.begin()->second; }

    const SweepLoopAnalysis* analysisForLoop(const RunBlock* loopRunBlock) const {
        auto result = _loops.find(loopRunBlock->getSequenceIndex());
        return (result != _loops.end()) ? result->second : nullptr;
    }

    bool hasTransitionForExit(const SweepLoopExit& loopExit) const {
        return _transitions.find(loopExit) != _transitions.end();
    }
    const SweepTransition* transitionForExit(const SweepLoopExit& loopExit) const {
        auto result = _transitions.find(loopExit);
        return (result != _transitions.end()) ? &(result->second) : nullptr;
    }
    void addTransitionForExit(const SweepLoopExit& loopExit, SweepTransition st) {
        _transitions[loopExit] = st;
    }
    int numTransitions() const { return (int)_transitions.size(); }

    void clear();
    bool analyzeLoop(SweepLoopAnalysis* loop, int runBlockIndex, const ProgramExecutor& executor);
    bool analyzeGroup();

    bool allOutsideDeltasMoveAwayFromZero(DataPointer dp, const Data& data) const;
    Trilian proofHang(DataPointer dp, const Data& data);
};

std::ostream &operator<<(std::ostream &os, const SweepTransitionGroup &group);

#endif /* SweepTransitionGroup_h */
