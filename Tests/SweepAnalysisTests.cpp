//
//  SweepAnalysisTests.cpp
//  Tests
//
//  Created by Erwin on 16/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#include "catch.hpp"

#include "MetaLoopAnalysis.h"
#include "ProgramBlock.h"
#include "RunUntilMetaLoop.h"
#include "SweepHangChecker.h"

const int dummySteps = 1;
const int maxSequenceLen = 16;

const int INC = true;
const int MOV = false;

// Sweep programs that may or may not hang.
TEST_CASE( "Meta-loop (sweeps)", "[meta-loop-analysis][sweep]" ) {
    HangExecutor hangExecutor(1000, 20000);
    hangExecutor.setMaxSteps(20000);
    hangExecutor.addHangDetector(std::make_shared<RunUntilMetaLoop>(hangExecutor, 6));

    SweepHangChecker hangChecker;

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
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 3);
        REQUIRE(lb.size() == 2);

        // Rightward sweep
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Leftward sweep
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(lb[1].maxDpDelta() == 1);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
    }

    SECTION( "SweepWithChangingBody" ) {
        // Sweep body increases by one each right sweep

        // Rightwards sweep
        block[0].finalize(MOV,  1, dummySteps, block + 2, block + 1);
        block[1].finalize(INC,  1, dummySteps, exitBlock, block + 0);

        // Transition sequence that extends sequence
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        // Leftwards sweep
        block[3].finalize(MOV, -1, dummySteps, block + 0, block + 3);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 3);
        REQUIRE(lb.size() == 2);

        // Rightward sweep
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Leftward sweep
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(lb[1].maxDpDelta() == 1);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
    }

    SECTION( "SweepWithStripedBody" ) {
        // Sweep body increases in length by two units. It's body consists of
        // alternating positive and negative values that diverge from zero.

        // Rightwards sweep
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  2, dummySteps, block + 2, block + 0);

        // Transition sequence that extends sequence
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, block + 4, exitBlock);

        // Leftwards sweep
        block[4].finalize(INC, -1, dummySteps, exitBlock, block + 5);
        block[5].finalize(MOV, -2, dummySteps, block + 6, block + 4);

        // Transition sequence
        block[6].finalize(MOV,  1, dummySteps, exitBlock, block + 0);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 4);
        REQUIRE(lb.size() == 2);

        // Leftward sweep
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 2);
        // Rightward sweep
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(lb[1].maxDpDelta() == 2);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
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
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 3);
        REQUIRE(lb.size() == 2);

        // Leftwards sweep
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == -1);
        REQUIRE(lb[0].maxDpDelta() == 0);
        // Rightward sweep
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == -1);
        REQUIRE(lb[1].maxDpDelta() == 0);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
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
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 4);
        REQUIRE(lb.size() == 2);

        // Rightward sweep
        REQUIRE(lb[0].loopType() == LoopType::DOUBLE_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 2);
        REQUIRE(lb[0].minDpDelta() == -1);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Leftward sweep
        REQUIRE(lb[1].loopType() == LoopType::DOUBLE_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 2);
        REQUIRE(lb[1].minDpDelta() == -1);
        REQUIRE(lb[1].maxDpDelta() == 1);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
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
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.metaLoopPeriod() == 6);
        REQUIRE(mla.loopSize() == 6);
        REQUIRE(lb.size() == 4);

        // Rightward sweep
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Leftward sweep
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(lb[1].maxDpDelta() == 1);
        // Rightward sweep
        REQUIRE(lb[2].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[2].iterationDelta() == 1);
        REQUIRE(lb[2].minDpDelta() == 0);
        REQUIRE(lb[2].maxDpDelta() == 1);
        // Leftward sweep
        REQUIRE(lb[3].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[3].iterationDelta() == 1);
        REQUIRE(lb[3].minDpDelta() == 0);
        REQUIRE(lb[3].maxDpDelta() == 1);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(!result);  // TODO: Support
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
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.metaLoopPeriod() == 3);
        REQUIRE(mla.loopSize() == 6);
        REQUIRE(lb.size() == 4);

        // Rightward sweep (followed by extend by two)
        REQUIRE(mla.loopIterationDelta(0) == 2);
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 3);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 3);
        // Leftward sweep
        REQUIRE(mla.loopIterationDelta(1) == 2);
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 3);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(lb[1].maxDpDelta() == 3);
        // Rightward sweep (followed by extend by one)
        REQUIRE(mla.loopIterationDelta(2) == 1);
        REQUIRE(lb[2].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[2].iterationDelta() == 3);
        REQUIRE(lb[2].minDpDelta() == 0);
        REQUIRE(lb[2].maxDpDelta() == 3);
        // Leftward sweep
        REQUIRE(mla.loopIterationDelta(3) == 1);
        REQUIRE(lb[3].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[3].iterationDelta() == 3);
        REQUIRE(lb[3].minDpDelta() == 0);
        REQUIRE(lb[3].maxDpDelta() == 3);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(!result);  // TODO: support
    }

    SECTION( "ConstantSweepBodyWithStationaryCounter" ) {
        // Sweep body consists of only ones and extends by one position to the right.
        // At its left is a counter that is incremented by one each meta-loop iteration.

        // Rightwards sweep
        block[0].finalize(MOV,  1, dummySteps, block + 1, block + 0);

        // Transition sequence that extends sequence
        block[1].finalize(INC,  1, dummySteps, exitBlock, block + 2);

        // Leftwards sweep
        block[2].finalize(MOV, -1, dummySteps, block + 3, block + 2);

        // Counter at left
        block[3].finalize(MOV, 1, dummySteps, exitBlock, block + 4);
        block[4].finalize(INC, 1, dummySteps, exitBlock, block + 0);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 4);
        REQUIRE(lb.size() == 2);

        // Rightward sweep
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Leftward sweep
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(lb[1].maxDpDelta() == 1);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
    }

    SECTION( "ConstantSweepBodyWithStationaryCounter2" ) {
        // Sweep body consists of only ones and extends by one position to the right.
        // At its left is a counter that is incremented by one each meta-loop iteration.
        // This is a result of the premature exit of the left sweep.

        // Rightwards sweep
        block[0].finalize(MOV,  1, dummySteps, block + 1, block + 0);

        // Transition sequence that extends sequence
        block[1].finalize(INC,  1, dummySteps, exitBlock, block + 2);

        // Leftwards sweep, which oscilates each value in the sweep body during traversal
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, block + 0, block + 4);
        block[4].finalize(MOV,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(INC, -1, dummySteps, exitBlock, block + 6);
        block[6].finalize(MOV, -1, dummySteps, exitBlock, block + 2);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 3);
        REQUIRE(lb.size() == 2);

        // Rightward sweep
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Leftward sweep
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(lb[1].maxDpDelta() == 1);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
    }

    SECTION( "BootstrappingSweep" ) {
        // The rightwards sweep is a loop with a bootstrap cycle. The bootstrap results in a
        // negative value moving away from zero, whereas the sweep body consists of positive values
        // that move away from zero.

        // Rightwards sweep
        block[0].finalize(INC, -1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 3, block + 2);
        block[2].finalize(INC,  2, dummySteps, exitBlock, block + 0);

        // Transition at right
        block[3].finalize(INC,  2, dummySteps, exitBlock, block + 4);

        // Leftwards sweep
        block[4].finalize(MOV, -1, dummySteps, block + 5, block + 4);

        // Transition at left
        block[5].finalize(MOV,  1, dummySteps, exitBlock, block + 0);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 4);
        REQUIRE(lb.size() == 2);

        // Rightward sweep
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Leftward sweep
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(lb[1].maxDpDelta() == 1);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
    }

    SECTION( "BootstrappingSweep2" ) {
        // The rightwards sweep is a loop with a bootstrap cycle. The changes during loop bootstrap
        // are negated by the transition sequence that precedes it.

        // Init
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 2);

        // Transition at left
        block[1].finalize(MOV,  1, dummySteps, exitBlock, block + 2);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        // Rightwards sweep
        block[3].finalize(INC, -1, dummySteps, exitBlock, block + 4);
        block[4].finalize(MOV,  1, dummySteps, block + 6, block + 5);
        block[5].finalize(INC,  2, dummySteps, exitBlock, block + 3);

        // Transition at right
        block[6].finalize(INC,  2, dummySteps, exitBlock, block + 7);

        // Leftwards sweep
        block[7].finalize(MOV, -1, dummySteps, block + 1, block + 7);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 4);
        REQUIRE(lb.size() == 2);

        // Rightward sweep
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Leftward sweep
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(lb[1].maxDpDelta() == 1);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
    }

    SECTION( "TwoPartSweep" ) {
        // The rightward sweep is a single loop, whereas the leftward sweep is broken in two
        // parts. The first part traverses the right part of the body, which consists of ones, and
        // the second part traverses the left part, consisting of minus ones.

        // Rightwards sweep
        block[0].finalize(MOV,  1, dummySteps, block + 1, block + 0);

        // Transition sequence extending sequence at right
        block[1].finalize(INC,  1, dummySteps, exitBlock, block + 2);

        // Leftwards sweep - Part 1
        block[2].finalize(MOV, -1, dummySteps, block + 5, block + 3);
        block[3].finalize(INC,  1, dummySteps, block + 5, block + 4);
        block[4].finalize(INC, -1, dummySteps, exitBlock, block + 2);

        // Mid-sequence transition
        block[5].finalize(INC, -1, dummySteps, exitBlock, block + 6);

        // Leftwards sweep - Part 2
        block[6].finalize(MOV, -1, dummySteps, block + 7, block + 6);

        // Transition sequence extending sequence at left
        block[7].finalize(INC, -1, dummySteps, exitBlock, block + 0);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 6);
        REQUIRE(lb.size() == 3);

        // Rightward sweep
        REQUIRE(lb[0].loopType() == LoopType::DOUBLE_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 2);
        REQUIRE(lb[0].minDpDelta() == -1);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Leftward sweep - Part 1
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(lb[1].maxDpDelta() == 1);
        // Leftward sweep - Part 2
        REQUIRE(lb[2].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[2].iterationDelta() == 1);
        REQUIRE(lb[2].minDpDelta() == -1);
        REQUIRE(lb[2].maxDpDelta() == 0);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(!result);  // TODO: support
    }

}
