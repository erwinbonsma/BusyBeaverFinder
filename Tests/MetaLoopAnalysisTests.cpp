//
//  MetaLoopAnalysisTests.cpp
//  Tests
//
//  Created by Erwin on 18/10/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "HangDetector.h"
#include "ProgramBlock.h"
#include "MetaLoopAnalysis.h"

const int dummySteps = 1;
const int maxSequenceLen = 12;

const int INC = true;
const int MOV = false;

class RunUntilMetaLoop : public HangDetector {
    int _numIterations;

protected:
    bool shouldCheckNow(bool loopContinues) const override {
//        if (!loopContinues) {
//            _execution.dumpExecutionState();
//        }

        return (!loopContinues
                && _execution.getMetaRunSummary().isInsideLoop()
                && _execution.getMetaRunSummary().getLoopIteration() >= _numIterations);
    }

    bool analyzeHangBehaviour() override { return true; };
    Trilian proofHang() override { return Trilian::YES; };

public:
    RunUntilMetaLoop(const ExecutionState& execution, int numIterations = 3)
    : HangDetector(execution), _numIterations(numIterations) {}
};

// Programs that end up in a permanent meta-loop
TEST_CASE( "Meta-loop (positive)", "[meta-loop-analysis][hang]" ) {
    HangExecutor hangExecutor(1000, 1000);
    hangExecutor.setMaxSteps(1000);
    hangExecutor.addHangDetector(std::make_shared<RunUntilMetaLoop>(hangExecutor, 6));

    ProgramBlock block[maxSequenceLen];
    for (int i = 0; i < maxSequenceLen; i++) {
        block[i].init(i);
    }
    ProgramBlock *exitBlock = &block[maxSequenceLen - 1];

    MetaLoopAnalysis mla;

    SECTION( "BasicRightSweep" ) {
        // Sweep body consists of only ones and extends by one position to the right

        // Rightwards sweep
        block[0].finalize(MOV,  1, dummySteps, block + 1, block + 0);

        // Transition sequence that extends sequence
        block[1].finalize(INC,  1, dummySteps, exitBlock, block + 2);

        // Leftwards sweep
        block[2].finalize(MOV, -1, dummySteps, block + 0, block + 2);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);

        REQUIRE(result == true);
        REQUIRE(mla.loopSize() == 3);
        REQUIRE(mla.loopIterationDelta(0) == 1);
        REQUIRE(mla.loopIterationDelta(2) == 1);
    }

    SECTION( "BasicLeftSweep" ) {
        // Sweep body consists of only ones and extends by one position to the left

        // Rightwards sweep
        block[0].finalize(MOV,  1, dummySteps, block + 1, block + 0);

        // Leftwards sweep
        block[1].finalize(MOV, -1, dummySteps, block + 2, block + 1);

        // Transition sequence that extends sequence at left
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 0);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);

        REQUIRE(result == true);
        REQUIRE(mla.loopSize() == 3);
        REQUIRE(mla.loopIterationDelta(0) == 1);
        REQUIRE(mla.loopIterationDelta(2) == 1);
    }

    SECTION( "BasicDualEndedSweep" ) {
        // Sweep body consists of only ones (at the right) and minus ones (at the left) and extends
        // by one position to the left and right
        block[0].finalize(MOV,  1, dummySteps, block + 1, block + 0);
        block[1].finalize(INC,  1, dummySteps, exitBlock, block + 2);
        block[2].finalize(MOV, -1, dummySteps, block + 3, block + 2);
        block[3].finalize(INC, -1, dummySteps, exitBlock, block + 0);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);

        REQUIRE(result == true);
        REQUIRE(mla.loopSize() == 4);
        REQUIRE(mla.loopIterationDelta(1) == 2);
        REQUIRE(mla.loopIterationDelta(3) == 2);
    }

    SECTION( "TwoStateRightSweep" ) {
        // Sweep that extends to the right once every two sweep iterations

        // Rightwards sweep loop
        block[0].finalize(MOV,  1, dummySteps, block + 4, block + 1); // Exit on zero
        block[1].finalize(INC, -1, dummySteps, block + 3, block + 2); // Exit on one
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 0);

        // One-exit
        block[3].finalize(INC,  1, dummySteps, exitBlock, block + 4);

        // Zero-exit
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);

        // Leftwards sweep loop
        block[5].finalize(MOV, -1, dummySteps, block + 0, block + 5);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);

        REQUIRE(result == true);
        REQUIRE(mla.loopSize() == 6);
        REQUIRE(mla.loopIterationDelta(0) == 1);
        REQUIRE(mla.loopIterationDelta(2) == 1);
        REQUIRE(mla.loopIterationDelta(3) == 1);
        REQUIRE(mla.loopIterationDelta(5) == 1);
    }

    SECTION( "TwoStateRightSweep-SharedTransition" ) {
        // Sweep that extends to the right by either one or two cells. This variation happens even
        // though the rightwards sweep loop always exits at the same instruction _and_ is followed
        // by the same transition sequence.

        // Rightwards sweep loop
        block[0].finalize(MOV,  1, dummySteps, block + 3, block + 1); // Exit on zero
        block[1].finalize(INC, -1, dummySteps, block + 3, block + 2); // Exit on one
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 0);

        // Transition sequence. Converts exit to a body, and adds one to each of the two cells at
        // the right
        block[3].finalize(INC,  2, dummySteps, exitBlock, block + 4);
        block[4].finalize(MOV,  1, dummySteps, block + 5, block + 5); // Shared exit
        block[5].finalize(INC,  1, dummySteps, exitBlock, block + 6);
        block[6].finalize(MOV,  1, dummySteps, block + 7, exitBlock);
        block[7].finalize(INC,  1, dummySteps, exitBlock, block + 8);

        // Leftwards sweep loop
        block[8].finalize(MOV, -1, dummySteps, block + 0, block + 8);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);

        REQUIRE(result == true);
        REQUIRE(mla.loopSize() == 6);
        REQUIRE(mla.loopIterationDelta(0) == 3);
        REQUIRE(mla.loopIterationDelta(2) == 3);
        REQUIRE(mla.loopIterationDelta(3) == 3);
        REQUIRE(mla.loopIterationDelta(5) == 3);
    }

    SECTION( "SimpleGlider" ) {
        // Glider counter increases by one each iteration

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 2, exitBlock);

        // Main loop
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, exitBlock, block + 4);
        block[4].finalize(INC, -1, dummySteps, block + 6, block + 5);
        block[5].finalize(MOV,  1, dummySteps, exitBlock, block + 2);

        // Transition
        block[6].finalize(MOV,  2, dummySteps, block + 7, exitBlock);
        block[7].finalize(INC,  1, dummySteps, exitBlock, block + 2);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);

        REQUIRE(result == true);
        REQUIRE(mla.loopSize() == 2);
        REQUIRE(mla.loopIterationDelta(1) == 1);
    }

    SECTION( "SimpleGliderDeltaTwo" ) {
        // Glider counter increases by two each iteration

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 2, exitBlock);

        // Main loop
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, exitBlock, block + 4);
        block[4].finalize(INC, -1, dummySteps, block + 6, block + 5);
        block[5].finalize(MOV,  1, dummySteps, exitBlock, block + 2);

        // Transition
        block[6].finalize(MOV,  2, dummySteps, block + 7, exitBlock);
        block[7].finalize(INC,  2, dummySteps, exitBlock, block + 2);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);

        REQUIRE(result == true);
        REQUIRE(mla.loopSize() == 2);
        REQUIRE(mla.loopIterationDelta(1) == 2);
    }

    SECTION( "PowersOfTwo" ) {
        // Calculates powers of two using two stationary counters that alternate between one being
        // decremented and the other being incremented

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 2, exitBlock);

        // Loop => C2_end = 2 * C1_start, C1_end = 0
        block[2].finalize(INC,  2, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, exitBlock, block + 4);
        block[4].finalize(INC, -1, dummySteps, block + 6, block + 5);
        block[5].finalize(MOV,  1, dummySteps, exitBlock, block + 2);

        // Loop => C1_end = C2_start, C2_end = 0
        block[6].finalize(INC,  1, dummySteps, exitBlock, block + 7);
        block[7].finalize(MOV,  1, dummySteps, exitBlock, block + 8);
        block[8].finalize(INC, -1, dummySteps, block + 2, block + 9);
        block[9].finalize(MOV, -1, dummySteps, exitBlock, block + 6);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);

        REQUIRE(result == true);
        REQUIRE(mla.loopSize() == 2);
        REQUIRE(mla.loopIterationDelta(0) == -1);
        REQUIRE(mla.loopIterationDelta(1) == -1);
    }
}

TEST_CASE( "Meta-loop (temporary, completion)", "[meta-loop-analysis][negative][completion]" ) {
    HangExecutor hangExecutor(1000, 1000);
    hangExecutor.setMaxSteps(1000);
    hangExecutor.addHangDetector(std::make_shared<RunUntilMetaLoop>(hangExecutor));

    ProgramBlock block[maxSequenceLen];
    for (int i = 0; i < maxSequenceLen; i++) {
        block[i].init(i);
    }
    ProgramBlock *exitBlock = &block[maxSequenceLen - 1];

    MetaLoopAnalysis mla;

    SECTION( "TerminatingGlider" ) {
        // Glider that starts with a positive loop counter that decreases each iteration

        // Bootstrap
        block[0].finalize(INC,  8, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 2, exitBlock);

        // Main loop
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, exitBlock, block + 4);
        block[4].finalize(INC, -1, dummySteps, block + 6, block + 5);
        block[5].finalize(MOV,  1, dummySteps, exitBlock, block + 2);

        // Transition
        block[6].finalize(MOV,  1, dummySteps, exitBlock, block + 7);
        block[7].finalize(INC, -1, dummySteps, exitBlock, block + 8);
        block[8].finalize(MOV,  1, dummySteps, block + 9, exitBlock);
        block[9].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);

        REQUIRE(result == false);
    }
}

// Examples of programs that do not end up in a permanent meta-periodic loop, but still hang
TEST_CASE( "Meta-loop (temporary, hang)", "[meta-loop-analysis][negative][hang]" ) {
    HangExecutor hangExecutor(1000, 1000);
    hangExecutor.setMaxSteps(1000);
    hangExecutor.addHangDetector(std::make_shared<RunUntilMetaLoop>(hangExecutor, 99));

    ProgramBlock block[maxSequenceLen];
    for (int i = 0; i < maxSequenceLen; i++) {
        block[i].init(i);
    }
    ProgramBlock *exitBlock = &block[maxSequenceLen - 1];

    MetaLoopAnalysis mla;

    SECTION( "IrregularSweep" ) {
        // Sweep body consists ones and twos and used to perform a binary count

        // Rightward sweep loop (exits on zero or one)
        block[0].finalize(MOV,  1, dummySteps, block + 3, block + 1);
        block[1].finalize(INC, -1, dummySteps, block + 5, block + 2);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 0);

        // Exit on zero
        block[3].finalize(INC,  1, dummySteps, exitBlock, block + 4);

        // Leftward sweep loop (only move)
        block[4].finalize(MOV, -1, dummySteps, block + 0, block + 4);

        // Exit on one
        block[5].finalize(INC,  2, dummySteps, exitBlock, block + 6);

        // Leftward sweep loop (toggle all twos to ones)
        block[6].finalize(MOV, -1, dummySteps, block + 0, block + 7);
        block[7].finalize(INC, -1, dummySteps, exitBlock, block + 6);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);

        // It does not consistently fail. It depends when analysis is executed. Only higher-level
        // analysis can reject it as a meta-periodic hang.
        REQUIRE(result == false);
    }

    SECTION( "CounterControlledSweep" ) {
        // The number of sweeps is controlled by a counter, which increases each time.

        // Init counter
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);

        // Rightwards sweep
        block[1].finalize(MOV,  1, dummySteps, block + 2, block + 1);

        // Extend sequence
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        // Leftwards sweep
        block[3].finalize(MOV, -1, dummySteps, block + 4, block + 3);

        block[4].finalize(MOV,  1, dummySteps, exitBlock, block + 5);

        // Decrease current counter and increase next
        block[5].finalize(INC, -1, dummySteps, block + 8, block + 6);
        block[6].finalize(MOV,  1, dummySteps, exitBlock, block + 7);
        block[7].finalize(INC,  1, dummySteps, exitBlock, block + 1);

        // Bump next
        block[8].finalize(MOV,  1, dummySteps, exitBlock, block + 9);
        block[9].finalize(INC,  2, dummySteps, exitBlock, block + 1);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);

        REQUIRE(result == false);
    }
}
