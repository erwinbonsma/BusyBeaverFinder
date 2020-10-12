//
//  LoopAnalysis.h
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef LoopAnalysis_h
#define LoopAnalysis_h

#include <stdio.h>

#include "SequenceAnalysis.h"

class InterpretedProgram;
class RunSummary;

const int maxLoopSize = 64;
const int maxLoopExits = maxSequenceSize;

enum class Operator : char {
    EQUALS = 0,
    UNEQUAL = 1,
    LESS_THAN_OR_EQUAL = 2,
    GREATER_THAN_OR_EQUAL = 3
};

enum class ExitWindow : char {
    // This exit can happen in any iteration of the loop
    ANYTIME = 0,

    // This exit can only happen while the loop is still bootstrapping
    BOOTSTRAP = 1,

    // This exit can never be taken (as long as the loop starts running from its first instruction)
    NEVER = 2
};

class ExitCondition {
    int _dpOffset;

    int _value;
    unsigned int _modulus;
    Operator _operator;

public:
    void init(Operator op, int value, int dpOffset);

    Operator getOperator() { return _operator; }
    void setOperator(Operator op) { _operator = op; }

    // An (optional) modulus constraint. This is required when DP is stationary and values increase
    // (or decrease) by more than one, as this may result in skipping the zero.
    unsigned int modulusConstraint() { return _modulus; }
    void setModulusConstraint(unsigned int modulus) { _modulus = modulus; }
    void clearModulusConstraint() { _modulus = 1; }

    // Checks if the condition holds for the specified value. It's the responsibility of the caller
    // to pass the correct value(s), i.e. one which the instruction that can cause this exit will
    // actually consume. To ensure this you need to use dpOffset().
    bool isTrueForValue(int value);

    // Specifies which data value the condition applies to. How to interpret this depends on whether
    // the DP is Travelling during loop execution. Let d = abs(dataPointerDelta)
    // - Travelling (d > 0): The offset gives the index in the d-sized window of new values that
    //   each loop consumes. It is always positive. The zero-index corresponds to DP before the
    //   first instruction executes.
    // - Stationary/Sentry Go (d = 0): The offset relative to the position of DP before the first
    //   instruction in the loop executes.
    int dpOffset() { return _dpOffset; }

    int value() { return _value; }

    bool expressionEquals(Operator op, int value) { return op == _operator && value == _value; }
    bool modulusContraintEquals(int modulus) { return modulus == _modulus; }

    void dump();
    void dumpWithoutEOL();
};

class LoopExit {
public:
    // Indicates that this exit is the first that is based on this value, once the loop is fully
    // bootstrapped.
    bool firstForValue;

    // Indicates when the exit may occur
    ExitWindow exitWindow;

    // When the exitWindow is BOOTSTRAP, the condition is wrt to the value at the start of a loop
    // iteration. When the exit window is ALWAYS, the condition is wrt to the value when it is
    // first consumed by the loop (once the loop has finished executing its bootstrap cycles)
    ExitCondition exitCondition;

    void dump();
};

class LoopAnalysis : public SequenceAnalysis {
    int _numBootstrapCycles;
    LoopExit _loopExit[maxLoopExits];

    // Returns true if the specified loop instruction exits the loop on a zero-value
    bool exitsOnZero(int index);

    // Determine the effective delta over multiple iterations, taking into account the shifting DP
    void squashDeltas();

    void setExitConditionsForStationaryLoop();
    void identifyBootstrapOnlyExitsForStationaryLoop();
    void markUnreachableExitsForStationaryLoop();
    void initExitsForStationaryLoop();

    void initExitsForTravellingLoop();

    void analyseSequence();
public:
    LoopAnalysis();

    int loopSize() { return _numBlocks; }

    // The number of iterations before the loop is fully spun up. A loop is spun up once it is
    // always the same loop instruction (or set of instructions) that first sees a data value.
    int numBootstrapCycles() { return _numBootstrapCycles; }

    // Returns the loop exit for the specified loop instruction
    LoopExit& exit(int index) { return _loopExit[index]; }

    // Analyses the loop. Returns true if analysis was successful.
    bool analyseLoop(ProgramBlock* entryBlock, int numBlocks);
    bool analyseLoop(InterpretedProgram& program, RunSummary& runSummary,
                     int startIndex, int period);

    void dump();
};

#endif /* LoopAnalysis_h */
