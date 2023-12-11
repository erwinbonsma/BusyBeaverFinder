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
#include "GliderHangChecker.h"

const int dummySteps = 1;
const int maxSequenceLen = 16;

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
    HangExecutor hangExecutor(1000, 20000);
    hangExecutor.setMaxSteps(20000);
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
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 2);
        REQUIRE(lb.size() == 1);

        // Glider loop
        REQUIRE(lb[0].loopType() == LoopType::GLIDER);
        REQUIRE(lb[0].iterationDelta() == 2);
        REQUIRE(lb[0].minDpDelta() == 1);
        REQUIRE(lb[0].maxDpDelta() == 1);
    }

    SECTION( "DoubleCounterGlider" ) {
        // Glider that increases two counters during each meta-loop iteration, thereby ensuring
        // that iterations go up non-linearly. This behavior is also harder to analyze, as the
        // next loop counter is incremented one less than the current counter.

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 5);

        // Main loop
        block[3].finalize(INC, -1, dummySteps, block + 9, block + 4);
        block[4].finalize(MOV,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(INC,  1, dummySteps, exitBlock, block + 6);
        block[6].finalize(MOV,  1, dummySteps, block + 7, block + 7);
        block[7].finalize(INC,  1, dummySteps, exitBlock, block + 8);
        block[8].finalize(MOV, -2, dummySteps, exitBlock, block + 3);

        // Transition
        block[9].finalize(MOV,  2, dummySteps, exitBlock, block + 5);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 2);
        REQUIRE(lb.size() == 1);

        // Glider loop
        REQUIRE(lb[0].loopType() == LoopType::GLIDER);
        REQUIRE(lb[0].iterationDelta() == -1);
        REQUIRE(lb[0].minDpDelta() == 1);
        REQUIRE(lb[0].maxDpDelta() == 1);
    }

    SECTION( "TwoIndependentGliders" ) {
        // Two chained gliders that independently update their counters. The first counter
        // increases by one each meta-loop iteration. The other counter doubles each time.
        // Data: C1_OLD C1_NEW C2_OLD C2_NEW

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  2, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, block + 4, exitBlock);

        // Glider 1
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(MOV, -1, dummySteps, exitBlock, block + 6);
        block[6].finalize(INC, -1, dummySteps, block + 8, block + 7);
        block[7].finalize(MOV,  1, dummySteps, exitBlock, block + 4);

        // Transition
        block[8].finalize(MOV,  3, dummySteps, block + 9, exitBlock);

        // Glider 2
        block[ 9].finalize(INC,  2, dummySteps, exitBlock, block + 10);
        block[10].finalize(MOV, -1, dummySteps, exitBlock, block + 11);
        block[11].finalize(INC, -1, dummySteps, block + 13, block + 12);
        block[12].finalize(MOV,  1, dummySteps, exitBlock, block +  9);

        // Transition
        block[13].finalize(INC,  1, dummySteps, exitBlock, block + 4);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 4);
        REQUIRE(lb.size() == 2);

        // Glider loop 1
        REQUIRE(lb[0].loopType() == LoopType::GLIDER);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 1);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Glider loop 1
        REQUIRE(lb[1].loopType() == LoopType::GLIDER);
        REQUIRE(lb[1].iterationDelta() == -1);
        REQUIRE(lb[1].minDpDelta() == 1);
        REQUIRE(lb[1].maxDpDelta() == 1);
    }

    SECTION( "TwoEntangledIndependentGliders" ) {
        // Two chained gliders that independently update their counters. The first counter
        // increases by one each meta-loop iteration. The other counter doubles each time.
        // Very similar to TwoIndependentGliders except that the data regions are now entangled.
        // Data: C1_OLD C2_OLD C1_NEW C2_NEW

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV,  1, dummySteps, block + 4, exitBlock);

        // Glider 1
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(MOV, -2, dummySteps, exitBlock, block + 6);
        block[6].finalize(INC, -1, dummySteps, block + 8, block + 7);
        block[7].finalize(MOV,  2, dummySteps, exitBlock, block + 4);

        // Transition
        block[8].finalize(MOV,  3, dummySteps, block + 9, exitBlock);

        // Glider 2
        block[ 9].finalize(INC,  2, dummySteps, exitBlock, block + 10);
        block[10].finalize(MOV, -2, dummySteps, exitBlock, block + 11);
        block[11].finalize(INC, -1, dummySteps, block + 13, block + 12);
        block[12].finalize(MOV,  2, dummySteps, exitBlock, block +  9);

        // Transition
        block[13].finalize(MOV,  3, dummySteps, block + 14, exitBlock);
        block[14].finalize(INC,  1, dummySteps, exitBlock, block +  4);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 4);
        REQUIRE(lb.size() == 2);

        // Glider loop 1
        REQUIRE(lb[0].loopType() == LoopType::GLIDER);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 2);
        REQUIRE(lb[0].maxDpDelta() == 2);
        // Glider loop 1
        REQUIRE(lb[1].loopType() == LoopType::GLIDER);
        REQUIRE(lb[1].iterationDelta() == -1);
        REQUIRE(lb[1].minDpDelta() == 2);
        REQUIRE(lb[1].maxDpDelta() == 2);
    }

    SECTION( "TwoDependentGliders" ) {
        // Two chained gliders with a dependency between their counters. The first loop updates its
        // own next counter by one each meta-loop iteration, but also updates the counter for the
        // other loop. The other counter doubles each time.
        // Data: C1_OLD C1_NEW C2_OLD C2_NEW

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  2, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, block + 4, exitBlock);

        // Glider 1
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5); // Update C1_NEW
        block[5].finalize(MOV,  1, dummySteps, exitBlock, block + 6);
        block[6].finalize(INC,  1, dummySteps, exitBlock, block + 7); // Update C2_OLD
        block[7].finalize(MOV, -2, dummySteps, exitBlock, block + 8);
        block[8].finalize(INC, -1, dummySteps, block + 10, block + 9); // Update C1_OLD
        block[9].finalize(MOV,  1, dummySteps, exitBlock, block + 4);

        // Transition
        block[10].finalize(MOV,  3, dummySteps, block + 11, exitBlock);

        // Glider 2
        block[11].finalize(INC,  2, dummySteps, exitBlock, block + 12); // Update C2_NEW
        block[12].finalize(MOV, -1, dummySteps, exitBlock, block + 13);
        block[13].finalize(INC, -1, dummySteps, block + 15, block + 14); // Update C2_OLD
        block[14].finalize(MOV,  1, dummySteps, exitBlock, block + 11);

        // Transition
        block[15].finalize(INC,  1, dummySteps, exitBlock, block + 4);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 4);
        REQUIRE(lb.size() == 2);

        // Glider loop 1
        REQUIRE(lb[0].loopType() == LoopType::GLIDER);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 1);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Glider loop 2
        REQUIRE(lb[1].loopType() == LoopType::GLIDER);
        REQUIRE(lb[1].iterationDelta() == -1);
        REQUIRE(lb[1].minDpDelta() == 1);
        REQUIRE(lb[1].maxDpDelta() == 1);
    }

    SECTION( "StationaryLoop-PowersOfTwo" ) {
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
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 2);
        REQUIRE(lb.size() == 2);

        // Stationary loop 1
        REQUIRE(lb[0].loopType() == LoopType::STATIONARY);
        REQUIRE(lb[0].iterationDelta() == -1);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 0);
        // Stationary loop 2
        REQUIRE(lb[1].loopType() == LoopType::STATIONARY);
        REQUIRE(lb[1].iterationDelta() == -1);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(lb[1].maxDpDelta() == 0);
    }

    SECTION( "SweepWithGlider" ) {
        // Sweep body consists of only ones and extends by one position to the right.
        // At its right is a moving counter that is incremented by one each meta-loop iteration.

        // Bootstrap sequence
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);

        // Rightwards sweep
        block[1].finalize(MOV,  1, dummySteps, block + 2, block + 1);

        // Transition towards glider - bootstrap counter
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        // Transition glider that extends sequence and increases moving counter
        block[3].finalize(INC,  1, dummySteps, exitBlock, block + 4);
        block[4].finalize(MOV, -1, dummySteps, exitBlock, block + 5);
        block[5].finalize(INC, -1, dummySteps, block + 7, block + 6);
        block[6].finalize(MOV,  1, dummySteps, exitBlock, block + 3);

        // Transition after glider - restore zero to one
        block[7].finalize(INC,  1, dummySteps, exitBlock, block + 8);

        // Leftwards sweep
        block[8].finalize(MOV, -1, dummySteps, block + 1, block + 8);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 5);
        REQUIRE(lb.size() == 3);

        // Rightward sweep
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(lb[0].maxDpDelta() == 1);
        // Glider counter
        REQUIRE(lb[1].loopType() == LoopType::GLIDER);
        REQUIRE(lb[1].iterationDelta() == 1);
        REQUIRE(lb[1].minDpDelta() == 1);
        REQUIRE(lb[1].maxDpDelta() == 1);
        // Leftward sweep
        REQUIRE(lb[2].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[2].iterationDelta() == 1);
        REQUIRE(lb[2].minDpDelta() == 0);
        REQUIRE(lb[2].maxDpDelta() == 1);
    }
}

// Glider programs that may or may not hang.
TEST_CASE( "Meta-loop (simple gliders)", "[meta-loop-analysis][glider]" ) {
    HangExecutor hangExecutor(1000, 20000);
    hangExecutor.setMaxSteps(20000);
    hangExecutor.addHangDetector(std::make_shared<RunUntilMetaLoop>(hangExecutor, 6));

    GliderHangChecker hangChecker;

    ProgramBlock block[maxSequenceLen];
    for (int i = 0; i < maxSequenceLen; i++) {
        block[i].init(i);
    }
    ProgramBlock *exitBlock = &block[maxSequenceLen - 1];

    MetaLoopAnalysis mla;

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
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 2);
        REQUIRE(lb.size() == 1);

        // Glider loop
        REQUIRE(lb[0].loopType() == LoopType::GLIDER);
        REQUIRE(lb[0].iterationDelta() == 1);
        REQUIRE(lb[0].minDpDelta() == 1);
        REQUIRE(lb[0].maxDpDelta() == 1);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
        REQUIRE(hangChecker.counterDpOffset() == -1);

        auto& tla = hangChecker.transitionLoopAnalysis();
        REQUIRE(tla.loopSize() == 4); // Sequence ends at at glider loop exit instruction
        REQUIRE(tla.dataPointerDelta() == 1);
    }

    SECTION( "GliderWithWake-PowersOfTwo" ) {
        // Glider that leaves powers of two in its wake
        // Data: Wake Now Next

        // Bootstrap: Now = 1
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 2, exitBlock);

        // Loop: Wake += 2, Now -= 1, Next += 2
        block[2].finalize(INC,  2, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -2, dummySteps, block + 4, block + 4);
        block[4].finalize(INC,  2, dummySteps, exitBlock, block + 5);
        block[5].finalize(MOV,  1, dummySteps, exitBlock, block + 6);
        block[6].finalize(INC, -1, dummySteps, block + 8, block + 7);
        block[7].finalize(MOV,  1, dummySteps, exitBlock, block + 2);

        // Transition
        block[8].finalize(MOV,  2, dummySteps, block + 2, exitBlock);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 2);
        REQUIRE(lb.size() == 1);

        // Glider loop
        REQUIRE(lb[0].loopType() == LoopType::GLIDER);
        REQUIRE(lb[0].iterationDelta() == -1);
        REQUIRE(lb[0].minDpDelta() == 1);
        REQUIRE(lb[0].maxDpDelta() == 1);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
        REQUIRE(hangChecker.counterDpOffset() == -1);

        auto& tla = hangChecker.transitionLoopAnalysis();
        REQUIRE(tla.loopSize() == 5); // Sequence ends at at glider loop exit instruction
        REQUIRE(tla.dataPointerDelta() == 1);
    }

    SECTION( "GliderWithWake-PowersOfThree" ) {
        // Glider that leaves powers of three in its wake
        // Data: Wake Now Next

        // Bootstrap: Now = 1
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 2, exitBlock);

        // Loop: Wake += 3, Now -= 1, Next += 3
        block[2].finalize(INC,  3, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -2, dummySteps, block + 4, block + 4);
        block[4].finalize(INC,  3, dummySteps, exitBlock, block + 5);
        block[5].finalize(MOV,  1, dummySteps, exitBlock, block + 6);
        block[6].finalize(INC, -1, dummySteps, block + 8, block + 7);
        block[7].finalize(MOV,  1, dummySteps, exitBlock, block + 2);

        // Transition
        block[8].finalize(MOV, -1, dummySteps, exitBlock, block + 9);
        block[9].finalize(INC, -1, dummySteps, exitBlock, block + 10);
        block[10].finalize(MOV, 3, dummySteps, block + 2, exitBlock);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        REQUIRE(result);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
        REQUIRE(hangChecker.counterDpOffset() == -1);

        auto& tla = hangChecker.transitionLoopAnalysis();
        REQUIRE(tla.loopSize() == 7); // Sequence ends at at glider loop exit instruction
        REQUIRE(tla.dataPointerDelta() == 1);
    }

    SECTION("Glider-TransitionClearsZeroesAhead") {
        // This glider program has a transition which writes ones several steps ahead of its
        // glider loop. This complicates checks that there are only zeroes ahead.

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV, -1, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  2, dummySteps, exitBlock, block + 3);

        // Glider loop
        block[3].finalize(INC, -1, dummySteps, block + 7, block + 4);
        block[4].finalize(MOV,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(INC,  2, dummySteps, exitBlock, block + 6);
        block[6].finalize(MOV, -1, dummySteps, exitBlock, block + 3);

        // Transition
        block[ 7].finalize(MOV,  8, dummySteps, block + 8, exitBlock);
        block[ 8].finalize(INC,  1, dummySteps, exitBlock, block + 9); // far-ahead data set
        block[ 9].finalize(MOV, -6, dummySteps, block + 10, exitBlock);
        block[10].finalize(INC,  1, dummySteps, exitBlock, block + 11); // init next counter
        block[11].finalize(MOV, -1, dummySteps, exitBlock, block + 3);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        REQUIRE(result);

        result = hangChecker.init(&mla, hangExecutor);
        REQUIRE(result);
        REQUIRE(hangChecker.counterDpOffset() == 0);

        auto& tla = hangChecker.transitionLoopAnalysis();
        REQUIRE(tla.loopSize() == 5); // Sequence ends at at glider loop exit instruction
        REQUIRE(tla.dataPointerDelta() == 1);
    }
}

TEST_CASE( "Meta-loop (temporary, completion)", "[meta-loop-analysis][negative][completion]" ) {
    HangExecutor hangExecutor(1000, 100000);
    hangExecutor.setMaxSteps(10000000);
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

        REQUIRE(!result);
    }

    SECTION( "SweepConsumingGliderWake" ) {
        // Sweep connected to glider that leaves powers of three (minus one) in its wake.
        // The leftward sweep subtracts seven of all values. This eventually causes the program
        // to terminate, as (3^6 - 1) is divisible by seven. It requires 104 sweeps to get to
        // zero which means that the glider counter is meanwhile at 3^(6+104) ~= 3 * 10^52

        // Loop: Wake += 3, Now -= 1, Next += 3
        block[0].finalize(INC,  3, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV, -2, dummySteps, block + 2, block + 2);
        block[2].finalize(INC,  3, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV,  1, dummySteps, block + 6, block + 4);
        block[4].finalize(INC, -1, dummySteps, block + 6, block + 5);
        block[5].finalize(MOV,  1, dummySteps, exitBlock, block + 0);

        // Transition
        block[6].finalize(MOV, -1, dummySteps, exitBlock, block + 7);
        block[7].finalize(INC, -1, dummySteps, exitBlock, block + 8);

        // Leftward sweep
        block[8].finalize(MOV, -1, dummySteps, block + 10, block + 9);
        block[9].finalize(INC, -7, dummySteps, exitBlock, block + 8);

        // Rightward sweep
        block[10].finalize(MOV,  1, dummySteps, block + 11, block + 10);

        // Transition at right
        block[11].finalize(MOV,  2, dummySteps, block + 0, exitBlock);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        REQUIRE(result);
        REQUIRE(mla.loopSize() == 5);
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
        REQUIRE(!result);
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

        REQUIRE(!result);
    }
}
