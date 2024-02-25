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

const bool INC = true;
const bool MOV = false;

// Sweep programs that may or may not hang.
TEST_CASE("Meta-loop (sweeps)", "[meta-loop-analysis][sweep]") {
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

    SECTION("BasicRightSweep") {
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

        auto& tl = hangChecker.leftTransition();
        REQUIRE(tl.transitionDeltas().size() == 0);

        auto& tr = hangChecker.rightTransition();
        auto& ddr = tr.transitionDeltas();
        REQUIRE(ddr.size() == 1);
        REQUIRE(ddr.deltaAt(0) == 1);

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.sweepLoopDeltas().size() == 0);
        REQUIRE(!hangChecker.rightSweepLoop());
    }

    SECTION("SweepWithChangingBody") {
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

        auto& tl = hangChecker.leftTransition();
        REQUIRE(tl.transitionDeltas().size() == 0);

        auto& tr = hangChecker.rightTransition();
        auto& tdr = tr.transitionDeltas();
        REQUIRE(tdr.size() == 1);
        REQUIRE(tdr.deltaAt(0) >= 1); // May include modification of next incoming sweep

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.deltaRange() == 1);
        auto& sdl = sll.sweepLoopDeltas();
        REQUIRE(sdl.deltaAt(0) == 1);
        REQUIRE(!hangChecker.rightSweepLoop());
    }

    SECTION("SweepWithStripedBody") {
        // Sweep body increases in length by two units. Its body consists of
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

        auto& tl = hangChecker.leftTransition();
        auto& tdl = tl.transitionDeltas();
        REQUIRE(tdl.size() == 1);
        REQUIRE(tdl.deltaAt(1) == 1);

        auto& tr = hangChecker.rightTransition();
        auto& tdr = tr.transitionDeltas();
        REQUIRE(tdr.size() >= 2); // How far analysis covers sweep body is not important
        REQUIRE(tdr.deltaAt(0) >= 1); // How many sweep deltas are included is not important
        REQUIRE(tdr.deltaAt(-1) <= -1);

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.deltaRange() == 2);
        auto& sdl = sll.sweepLoopDeltas();
        REQUIRE(sdl.deltaAt(0) == 1);
        REQUIRE(sdl.deltaAt(1) == -1);
        REQUIRE(!hangChecker.rightSweepLoop());
    }

    SECTION("SweepWithShiftingBodyUpdates") {
        // The leftward sweep makes two different modifications: -1 and +3. Their positions,
        // however, shift by one each iteration of the meta-loop, so the effective delta is +2.

        // Transition sequence that extends sequence (at right)
        block[0].finalize(INC,  3, dummySteps, exitBlock, block + 1);

        // Leftwards sweep
        block[1].finalize(MOV, -1, dummySteps, block + 5, block + 2);
        block[2].finalize(INC, -1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, block + 5, block + 4);
        block[4].finalize(INC,  3, dummySteps, exitBlock, block + 1);

        // Rightwards sweep
        block[5].finalize(MOV,  1, dummySteps, block + 0, block + 5);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 6);
        REQUIRE(lb.size() == 4);

        // The loop behaviors are nearly identical
        for (int i = 0; i < 4; ++i) {
            REQUIRE(lb[i].loopType() == LoopType::ANCHORED_SWEEP);
            REQUIRE(lb[i].minDpDelta() == 0);
            REQUIRE(lb[i].maxDpDelta() == 2);
            // The loop-body (and DP delta) differs for the leftward and rightward sweep. Therefore
            // the iteration delta differs
            REQUIRE(lb[i].iterationDelta() == ((i + 1) % 2) + 1);
        }

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);

        auto& tl = hangChecker.leftTransition();
        auto& ddl = tl.transitionDeltas();
        // Note: This position depends on which of the two-incoming loop exits is used as
        // reference, so it is a bit arbitrary. The position can either be 0 or 1.
        REQUIRE(ddl.deltaAt(1) == 2);

        auto& tr = hangChecker.rightTransition();
        auto& ddr = tr.transitionDeltas();
        REQUIRE(ddr.size() == 2);
        REQUIRE(ddr.deltaAt(0) == 2);
        REQUIRE(ddr.deltaAt(1) == 3);

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.deltaRange() == 2);
        auto& sdl = sll.sweepLoopDeltas();
        REQUIRE(sdl.deltaAt(0) == 2);
        REQUIRE(sdl.deltaAt(1) == 2);
    }

    SECTION("BasicLeftSweep") {
        // Sweep body consists of only ones and extends by one position to the left

        // Rightwards sweep
        block[0].finalize(MOV,  1, dummySteps, block + 1, block + 0);

        // Leftwards sweep
        block[1].finalize(MOV, -1, dummySteps, block + 2, block + 1);

        // Transition sequence that extends sequence at left
        block[2].finalize(INC,  3, dummySteps, exitBlock, block + 0);

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

        auto& tl = hangChecker.leftTransition();
        auto& tdl = tl.transitionDeltas();
        REQUIRE(tdl.size() == 1);
        REQUIRE(tdl.deltaAt(0) == 3);

        auto& tr = hangChecker.rightTransition();
        REQUIRE(tr.transitionDeltas().size() == 0);

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.sweepLoopDeltas().size() == 0);
    }

    SECTION("BasicDualEndedSweep") {
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

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.sweepLoopDeltas().size() == 0);
    }

    SECTION("TwoStateRightSweep") {
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

        // All sweep loops behave the same
        for (int i = 0; i < 4; ++i) {
            REQUIRE(lb[i].loopType() == LoopType::ANCHORED_SWEEP);
            REQUIRE(lb[i].iterationDelta() == 1);
            REQUIRE(lb[i].minDpDelta() == 0);
            REQUIRE(lb[i].maxDpDelta() == 1);
        }

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);

        auto& tl = hangChecker.leftTransition();
        auto& tdl = tl.transitionDeltas();
        REQUIRE(tdl.size() == 0);

        auto& tr = hangChecker.rightTransition();
        auto& tdr = tr.transitionDeltas();
        REQUIRE(tdr.size() == 1);
        REQUIRE(tdr.deltaAt(0) == 2);

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.sweepLoopDeltas().size() == 0);
    }

    SECTION("TwoStateRightSweepWithSharedTransition") {
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
        block[4].finalize(MOV,  1, dummySteps, block + 5, block + 6);

        // Although the blocks are identical, they need to be different instances so that the exit
        // condition can be correctly determined
        block[5].finalize(INC,  1, dummySteps, exitBlock, block + 7);
        block[6].finalize(INC,  1, dummySteps, exitBlock, block + 7);

        block[7].finalize(MOV,  1, dummySteps, block + 8, exitBlock);
        block[8].finalize(INC,  1, dummySteps, exitBlock, block + 9);

        // Leftwards sweep loop
        block[9].finalize(MOV, -1, dummySteps, block + 0, block + 9);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.metaLoopPeriod() == 6);
        REQUIRE(mla.loopSize() == 6);
        REQUIRE(lb.size() == 4);

        // Rightward sweep (followed by extend by two)
        REQUIRE(mla.loopIterationDelta(0) == 3);
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 3);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 3);
        // Leftward sweep
        REQUIRE(mla.loopIterationDelta(1) == 3);
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 3);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(lb[1].maxDpDelta() == 3);
        // Rightward sweep (followed by extend by one)
        REQUIRE(mla.loopIterationDelta(2) == 3);
        REQUIRE(lb[2].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[2].iterationDelta() == 3);
        REQUIRE(lb[2].minDpDelta() == 0);
        REQUIRE(lb[2].maxDpDelta() == 3);
        // Leftward sweep
        REQUIRE(mla.loopIterationDelta(3) == 3);
        REQUIRE(lb[3].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[3].iterationDelta() == 3);
        REQUIRE(lb[3].minDpDelta() == 0);
        REQUIRE(lb[3].maxDpDelta() == 3);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);

        auto& tl = hangChecker.leftTransition();
        auto& tdl = tl.transitionDeltas();
        REQUIRE(tdl.size() == 0);

        auto& tr = hangChecker.rightTransition();
        auto& tdr = tr.transitionDeltas();
        REQUIRE(tdr.size() == 3);
        REQUIRE(tdr.deltaAt(0) == 2);
        REQUIRE(tdr.deltaAt(1) == 2);
        REQUIRE(tdr.deltaAt(2) == 2);

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.sweepLoopDeltas().size() == 0);
    }

    SECTION("ConstantSweepBodyWithStationaryCounter") {
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

        auto& tl = hangChecker.leftTransition();
        auto& tdl = tl.transitionDeltas();
        REQUIRE(tdl.size() == 1);
        REQUIRE(tdl.deltaAt(1) == 1);

        auto& tr = hangChecker.rightTransition();
        auto& tdr = tr.transitionDeltas();
        REQUIRE(tdr.size() == 1);
        REQUIRE(tdr.deltaAt(0) == 1);

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.sweepLoopDeltas().size() == 0);
    }

    SECTION("ConstantSweepBodyWithStationaryCounter2") {
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

        auto& tl = hangChecker.leftTransition();
        auto& tdl = tl.transitionDeltas();
        REQUIRE(tdl.size() == 1);
        REQUIRE(tdl.deltaAt(1) == 1);

        auto& tr = hangChecker.rightTransition();
        auto& tdr = tr.transitionDeltas();
        REQUIRE(tdr.size() == 1);
        REQUIRE(tdr.deltaAt(0) == 1);

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.sweepLoopDeltas().size() == 0);
    }

    SECTION("BootstrappingSweep") {
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

    SECTION("BootstrappingSweep2") {
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

    SECTION("TwoPartSweep") {
        // The rightward sweep is a single loop, whereas the leftward sweep is broken in two
        // parts. The first part traverses the right part of the body, which consists of positive
        // values that increase, and the second part traverses the left part, consisting of minus
        // ones.

        // Rightwards sweep
        block[0].finalize(MOV,  1, dummySteps, block + 1, block + 0);

        // Transition sequence extending sequence at right
        block[1].finalize(INC,  1, dummySteps, exitBlock, block + 2);

        // Leftwards sweep - Part 1
        block[2].finalize(MOV, -1, dummySteps, block + 4, block + 3);
        block[3].finalize(INC,  1, dummySteps, block + 4, block + 2);

        // Mid-sequence transition
        block[4].finalize(INC, -1, dummySteps, exitBlock, block + 5);

        // Leftwards sweep - Part 2
        block[5].finalize(MOV, -1, dummySteps, block + 6, block + 5);

        // Transition sequence extending sequence at left
        block[6].finalize(INC, -1, dummySteps, exitBlock, block + 0);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 6);
        REQUIRE(lb.size() == 3);

        // Rightward sweep
        REQUIRE(lb[2].loopType() == LoopType::DOUBLE_SWEEP);
        REQUIRE(lb[2].iterationDelta() == 2);
        REQUIRE(lb[2].minDpDelta() == -1);
        REQUIRE(lb[2].maxDpDelta() == 1);
        // Leftward sweep - Part 1
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Leftward sweep - Part 2
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == -1);
        REQUIRE(lb[1].maxDpDelta() == 0);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);

        auto& tl = hangChecker.leftTransition();
        auto& tdl = tl.transitionDeltas();
        REQUIRE(tdl.size() == 1);
        REQUIRE(tdl.deltaAt(0) == -1);

        auto& tr = hangChecker.rightTransition();
        auto& tdr = tr.transitionDeltas();
        REQUIRE(tdr.size() == 1);
        REQUIRE(tdr.deltaAt(0) == 1);

        auto& tm = hangChecker.midTransition();
        REQUIRE(tm);
        REQUIRE(tm.value().transitionDeltas().size() == 0);

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.deltaRange() == 1);
        auto& sdl = sll.sweepLoopDeltas();
        REQUIRE(sdl.deltaAt(0) == 0);
        auto& slr = hangChecker.rightSweepLoop();
        REQUIRE(slr);
        REQUIRE(slr.value().deltaRange() == 1);
        auto& sdr = slr.value().sweepLoopDeltas();
        REQUIRE(sdr.deltaAt(0) == 1);
    }

    SECTION("TerminatingSweepWithStripedBody") {
        // The rightward sweep is a loop that moves DP two positions. One cell it decrements, the
        // other is constant and terminates the sweep. However, there's a counter at the right
        // which causes the sweep loop to eventually exit abnormally, breaking the hang.

        // Bootstrap
        block[0].finalize(INC, 16, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV, -1, dummySteps, block + 7, exitBlock);

        // Rightwards sweep
        block[3].finalize(INC, -1, dummySteps, exitBlock, block + 4);
        block[4].finalize(MOV,  1, dummySteps, block + 6, block + 5);
        block[5].finalize(MOV,  1, dummySteps, exitBlock, block + 3);

        // Leftwards sweep
        block[6].finalize(MOV, -1, dummySteps, block + 7, block + 6);

        // Transition sequence extending sequence at left
        block[7].finalize(INC,  1, dummySteps, exitBlock, block + 8);
        block[8].finalize(MOV, -1, dummySteps, block + 9, exitBlock);
        block[9].finalize(INC, -1, dummySteps, exitBlock, block + 3);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 3);
        REQUIRE(lb.size() == 2);

        // Leftward sweep
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 2);
        REQUIRE(lb[0].minDpDelta() == -2);
        REQUIRE(lb[0].maxDpDelta() == 0);
        // Rightward sweep
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == -2);
        REQUIRE(lb[1].maxDpDelta() == 0);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
    }

    SECTION("SweepTerminationOnMidSweepTransition") {
        // Sweep with a mid-sweep transition, which decreases a counter and terminates when this
        // counter reaches zero.

        // Bootstrap
        block[0].finalize(INC, 16, dummySteps, exitBlock, block + 1);

        // Rightwards sweep
        block[1].finalize(MOV,  1, dummySteps, block + 2, block + 1);

        // Transition sequence, extending sequence by one
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        // Leftwards sweep (on right side)
        block[3].finalize(INC, -1, dummySteps, block + 4, block + 6);
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(MOV, -1, dummySteps, exitBlock, block + 3);

        // Leftwards sweep (on left side)
        block[6].finalize(MOV, -1, dummySteps, block + 7, block + 6);

        // Transition sequence extending sequence at left
        block[7].finalize(INC,  1, dummySteps, exitBlock, block + 1);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        REQUIRE(result);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);

        auto& tl = hangChecker.leftTransition();
        auto& tdl = tl.transitionDeltas();
        REQUIRE(tdl.size() == 1);
        REQUIRE(tdl.deltaAt(0) == 1);

        auto& tr = hangChecker.rightTransition();
        auto& tdr = tr.transitionDeltas();
        REQUIRE(tdr.size() == 1);
        REQUIRE(tdr.deltaAt(0) == 1);

        auto& tm = hangChecker.midTransition();
        REQUIRE(tm);
        auto& tdm = tm.value().transitionDeltas();
        REQUIRE(tdm.size() == 1);
        REQUIRE(tdm.deltaAt(0) == -1);
    }

    SECTION("SweepWithTwoDependentStationaryEndTransitions") {
        // The sweep extends to the left by one unit each time. The rightward sweep moves two
        // units. It alternates between two exits, A and B, which are at different data cells and
        // also exit at a different loop instruction. The transition sequences at the right are
        // dependent. The sentinel for exit A is restored to its value by the transition sequence
        // that follows exit B.

        // Bootstrap
        block[0].finalize(INC, -2, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV, -1, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC, -1, dummySteps, exitBlock, block + 3);

        // Leftward sweep
        block[3].finalize(MOV, -1, dummySteps, block + 4, block + 3);

        // Transition - extends sequence at left
        block[4].finalize(INC, 1, dummySteps, exitBlock, block + 5);

        // Rightward sweep
        block[5].finalize(INC, 1, dummySteps, block + 9, block + 6);  // exit B
        block[6].finalize(INC, 1, dummySteps, block + 8, block + 7);  // exit A
        block[7].finalize(MOV, 2, dummySteps, exitBlock, block + 5);

        // Transition A (partially restores own sentinel)
        block[ 8].finalize(INC, -1, dummySteps, exitBlock, block + 3);

        // Transition B (restores own sentinel _and_ sentinel for exit A)
        block[ 9].finalize(INC, -1, dummySteps, exitBlock, block + 10);
        block[10].finalize(MOV,  1, dummySteps, exitBlock, block + 11);
        block[11].finalize(INC, -1, dummySteps, exitBlock, block +  3);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 8);
        REQUIRE(lb.size() == 4);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);

        auto& tl = hangChecker.leftTransition();
        auto& tdl = tl.transitionDeltas();
        REQUIRE(tdl.size() == 2);
        REQUIRE(tdl.deltaAt(0) == 3);
        REQUIRE(tdl.deltaAt(-1) == 3);

        auto& tr = hangChecker.rightTransition();
        auto& tdr = tr.transitionDeltas();
        REQUIRE(tdr.size() == 0);

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.deltaRange() == 2);
        auto& sdl = sll.sweepLoopDeltas();
        REQUIRE(sdl.deltaAt(0) == 2);
        REQUIRE(sdl.deltaAt(1) == 2);
        REQUIRE(!hangChecker.rightSweepLoop());
    }

    SECTION("SweepWithTwoStationaryEndTransitions") {
        // The sweep extends to the left by one unit each time. The rightward sweep moves one
        // unit and increments all cells during its traversal. It always exits at the same
        // loop instruction, but alternates between two different data positions. The subsequent
        // transition sequences diverge to restore the sweep's tail so that it keeps alternating
        // between both sequences. The tail ends are respectively A = [... -1 1 0] and
        // B = [... -2 -1 0].

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV, -1, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC, -1, dummySteps, exitBlock, block + 3);

        // Leftward sweep
        block[3].finalize(MOV, -1, dummySteps, block + 4, block + 3);

        // Transition - extends sequence at left
        block[4].finalize(INC, 1, dummySteps, exitBlock, block + 5);

        // Rightward sweep
        block[5].finalize(INC, 1, dummySteps, block + 7, block + 6);
        block[6].finalize(MOV, 1, dummySteps, exitBlock, block + 5);

        // Transition sequence (right)
        block[7].finalize(MOV, 1, dummySteps, block + 11, block + 8);

        // Transition A: ... -1 1 0 => ... 0 [1] 0 => ... -2 -1 0
        block[ 8].finalize(INC, -2, dummySteps, exitBlock, block + 9);
        block[ 9].finalize(MOV, -1, dummySteps, block + 10, exitBlock);
        block[10].finalize(INC, -2, dummySteps, exitBlock, block + 3);

        // Transition B: ... -2 -1 0 => ... -1 0 [0] => ... -1 1 0
        block[11].finalize(MOV, -1, dummySteps, block + 12, exitBlock);
        block[12].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 8);
        REQUIRE(lb.size() == 4);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);

        auto& tl = hangChecker.leftTransition();
        auto& tdl = tl.transitionDeltas();
        REQUIRE(tdl.size() == 2);
        REQUIRE(tdl.deltaAt(0) == 3);
        REQUIRE(tdl.deltaAt(-1) == 2);

        auto& tr = hangChecker.rightTransition();
        auto& tdr = tr.transitionDeltas();
        REQUIRE(tdr.size() == 0);

        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.deltaRange() == 1);
        auto& sdl = sll.sweepLoopDeltas();
        REQUIRE(sdl.deltaAt(0) == 2);
        REQUIRE(!hangChecker.rightSweepLoop());
    }

    SECTION("SweepLooksBeyondSweepExit") {
        // The right sweep looks beyond the zero value that will cause it to exit, which must be
        // non-zero. After the sweep terminates, the sweep extends one position to the right,
        // again with a zero followed by a one. The sweep also extends one position to the left
        // each time. Together this ensures that look-ahead is always non-zero.

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  2, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, block + 4, exitBlock);

        // Left sweep
        block[4].finalize(MOV, -1, dummySteps, block + 5, block + 4);

        // Extend at left
        block[5].finalize(INC,  1, dummySteps, exitBlock, block + 6);

        // Right sweep
        block[6].finalize(MOV,  3, dummySteps, exitBlock, block + 7); // Look ahead, non-zero
        block[7].finalize(MOV, -1, dummySteps, block + 8, block + 6); // Sweep exit

        // Extend at right, preserving invariant
        block[ 8].finalize(INC,  1, dummySteps, exitBlock, block + 9);
        block[ 9].finalize(MOV,  1, dummySteps, exitBlock, block + 10);
        block[10].finalize(INC, -1, dummySteps, block + 11, exitBlock);
        block[11].finalize(MOV,  1, dummySteps, block + 12, exitBlock);
        block[12].finalize(INC,  1, dummySteps, exitBlock, block + 13);
        block[13].finalize(MOV, -2, dummySteps, exitBlock, block + 4);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);

        // TODO: Check analysis
    }
}

TEST_CASE("Meta-loop (sweep loop analysis)", "[meta-loop-analysis][sweep]") {
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

    SECTION("SweepWithComplexSweepLoop") {
        // Sweep with a sweep loop that is more difficult to analyze.
        // The rightwards sweep looks ahead several positions, so requires combining multiple
        // loop iterations to get the effect.

        // Bootsrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 7);

        // Rightwards sweep loop
        block[3].finalize(INC,  1, dummySteps, exitBlock, block + 4);
        block[4].finalize(MOV,  3, dummySteps, block + 7, block + 5);
        block[5].finalize(INC,  2, dummySteps, exitBlock, block + 6);
        block[6].finalize(MOV, -2, dummySteps, block + 9, block + 3);

        // Leftwards sweep loop
        block[7].finalize(INC,  4, dummySteps, exitBlock, block + 8);
        block[8].finalize(MOV, -1, dummySteps, block + 9, block + 7);

        // Extension
        block[9].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 3);
        REQUIRE(lb.size() == 2);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.deltaRange() == 1);
        auto& sdl = sll.sweepLoopDeltas();
        REQUIRE(sdl.deltaAt(0) == 7);
    }

    SECTION("SweepWithComplexSweepLoop2") {
        // Sweep with a sweep loop that is more difficult to analyze. The program is similar to
        // SweepWithComplexSweepLoop, except that the leftward sweep now moves DP two positions.
        // This means that it only modifies half of the cells it traverses. However, as the sweep
        // loop itself shifts its starting point each meta-loop iteration, the overall effect on
        // the sweep body is still uniformly distributed.

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV, -1, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, block + 4, exitBlock);

        // Rightwards sweep loop
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(MOV,  3, dummySteps, block + 8, block + 6);
        block[6].finalize(INC,  2, dummySteps, exitBlock, block + 7);
        block[7].finalize(MOV, -2, dummySteps, exitBlock, block + 4);

        // Extension at right
        block[8].finalize(INC,  1, dummySteps, exitBlock, block + 9);

        // Leftwards sweep loop
        block[ 9].finalize(INC,  4, dummySteps, exitBlock, block + 10);
        block[10].finalize(MOV, -2, dummySteps, block + 11, block + 9);

        // Extension at left
        block[11].finalize(INC,  1, dummySteps, exitBlock, block + 4);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 4);
        REQUIRE(lb.size() == 2);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.deltaRange() == 2);
        auto& sdl = sll.sweepLoopDeltas();
        REQUIRE(sdl.deltaAt(0) == 3);
        REQUIRE(sdl.deltaAt(1) == 7);
    }

    SECTION("SweepWithComplexSweepLoop3") {
        // Sweep with a sweep loop that is more difficult to analyze. The program is similar to
        // SweepWithComplexSweepLoop2, except that the sweep only extends at the right (by two
        // cells). This means that the assymetric update of the leftward sweep now remains fixed.

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV, -1, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, block + 13, exitBlock);

        // Rightwards sweep loop
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(MOV,  3, dummySteps, block + 8, block + 6);
        block[6].finalize(INC,  1, dummySteps, exitBlock, block + 7);
        block[7].finalize(MOV, -2, dummySteps, exitBlock, block + 4);

        // Extension at right
        block[ 8].finalize(INC,  1, dummySteps, exitBlock, block +  9);
        block[ 9].finalize(MOV,  1, dummySteps, block + 10, exitBlock);
        block[10].finalize(INC,  1, dummySteps, exitBlock, block + 11);

        // Leftwards sweep loop
        block[11].finalize(INC, -5, dummySteps, exitBlock, block + 12);
        block[12].finalize(MOV, -2, dummySteps, block + 5, block + 11);

        // More bootstrap
        block[13].finalize(INC,  1, dummySteps, exitBlock, block + 14);
        block[14].finalize(MOV, -1, dummySteps, block + 4, exitBlock);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 3);
        REQUIRE(lb.size() == 2);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
        auto& sll = hangChecker.leftSweepLoop();
        REQUIRE(sll.deltaRange() == 2);
        auto& sdl = sll.sweepLoopDeltas();
        REQUIRE(sdl.deltaAt(0) == -3);
        REQUIRE(sdl.deltaAt(1) == 2);
    }
}

TEST_CASE("Meta-loop (sweep transitions)", "[meta-loop-analysis][sweep]") {
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

    SECTION("StationaryTransitionAtLeftOfSweep") {
        // Stationary transition sequence at left with a large DP-range for testing purposes.

        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV, -7, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV,  4, dummySteps, block + 6, exitBlock);

        // Rightwards sweep, increments one
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(MOV,  1, dummySteps, block + 6, block + 4);

        // Leftwards sweep, increments two
        block[6].finalize(INC,  2, dummySteps, exitBlock, block + 7);
        block[7].finalize(MOV, -1, dummySteps, block + 8, block + 6);

        // Stationary transition under test
        block[ 8].finalize(MOV,  4, dummySteps, exitBlock, block +  9);
        block[ 9].finalize(INC,  3, dummySteps, exitBlock, block + 10);
        block[10].finalize(MOV, -7, dummySteps, exitBlock, block + 11);
        block[11].finalize(INC,  4, dummySteps, exitBlock, block + 12);
        block[12].finalize(MOV,  4, dummySteps, exitBlock, block +  4);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 3);
        REQUIRE(lb.size() == 2);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);

        auto &stg = hangChecker.leftTransition();
        REQUIRE(stg.isStationary());

        auto &dd = stg.transitionDeltas();
        REQUIRE(dd.size() == 5);
        REQUIRE(dd.deltaAt(-3) == 4);
        REQUIRE(dd.deltaAt( 1) == 3);
        REQUIRE(dd.deltaAt( 2) == 3);
        REQUIRE(dd.deltaAt( 3) == 3);
        REQUIRE(dd.deltaAt( 4) == 6);
    }

    SECTION("StationaryTransitionAtRightOfSweep") {
        // Basically an inverse of StationaryTransitionAtLeftOfSweep

        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  7, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -4, dummySteps, block + 6, exitBlock);

        // Leftwards sweep, increments one
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(MOV, -1, dummySteps, block + 6, block + 4);

        // Rightwards sweep, increments two
        block[6].finalize(INC,  2, dummySteps, exitBlock, block + 7);
        block[7].finalize(MOV,  1, dummySteps, block + 8, block + 6);

        // Stationary transition under test
        block[ 8].finalize(MOV, -4, dummySteps, exitBlock, block +  9);
        block[ 9].finalize(INC,  3, dummySteps, exitBlock, block + 10);
        block[10].finalize(MOV,  7, dummySteps, exitBlock, block + 11);
        block[11].finalize(INC,  4, dummySteps, exitBlock, block + 12);
        block[12].finalize(MOV, -4, dummySteps, exitBlock, block +  4);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 3);
        REQUIRE(lb.size() == 2);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);

        auto &stg = hangChecker.rightTransition();
        REQUIRE(stg.isStationary());

        auto &dd = stg.transitionDeltas();
        REQUIRE(dd.size() == 5);
        REQUIRE(dd.deltaAt( 3) == 4);
        REQUIRE(dd.deltaAt(-1) == 3);
        REQUIRE(dd.deltaAt(-2) == 3);
        REQUIRE(dd.deltaAt(-3) == 3);
        REQUIRE(dd.deltaAt(-4) == 6);
    }
}
