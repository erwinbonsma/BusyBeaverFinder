//
//  IrregularSweepAnalysisTests.cpp
//  Tests
//
//  Created by Erwin on 29/04/2024.
//  Copyright © 2024 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "MetaLoopAnalysis.h"
#include "ProgramBlock.h"
#include "RunUntilMetaLoop.h"
#include "IrregularSweepHangChecker.h"

const int dummySteps = 1;
const int maxSequenceLen = 16;

const bool INC = true;
const bool MOV = false;

// Sweep programs that may or may not hang.
TEST_CASE("Hang analysis (irregular sweeps)", "[hang-analysis][sweep][blocks][irregular]") {
    constexpr int maxSteps = 10000;
    HangExecutor hangExecutor(1000, maxSteps);
    hangExecutor.addHangDetector(std::make_shared<RunUntilMetaMetaLoop>(hangExecutor, 6));

    IrregularSweepHangChecker hangChecker;

    ProgramBlock block[maxSequenceLen];
    for (int i = 0; i < maxSequenceLen; i++) {
        block[i].init(i);
    }
    ProgramBlock *exitBlock = &block[maxSequenceLen - 1];

    MetaLoopAnalysis mla;
    IrregularSweepHangChecker checker;

    SECTION("BasicIrregularSweep") {
        // Irregular sweep appendix at right side values 1 and 2. The first 1 exits the sweep and
        // toggles to 2. All swept 2's toggle to 1 on sweep return.

        // Rightward sweep loop (exits on zero or one)
        block[0].finalize(MOV,  1, dummySteps, block + 4, block + 1);
        block[1].finalize(INC, -1, dummySteps, block + 3, block + 2);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 0);

        // Exit on one
        block[3].finalize(INC,  2, dummySteps, exitBlock, block + 5);

        // Exit on zero (extends sweep)
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);

        // Leftward sweep loop (toggle all twos to ones)
        block[5].finalize(MOV, -1, dummySteps, block + 7, block + 6);
        block[6].finalize(INC, -1, dummySteps, exitBlock, block + 5);

        // Transition at left (extends sweep)
        block[7].finalize(INC, -1, dummySteps, exitBlock, block + 0);

        auto program = std::make_shared<InterpretedProgramFromArray>(block, maxSequenceLen);
        hangExecutor.execute(program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        REQUIRE(result);
        REQUIRE(mla.metaLoopType() == MetaLoopType::IRREGULAR);
        REQUIRE(mla.loopSize() == 4);

        auto lb = mla.loopBehaviors();
        REQUIRE(lb.size() == 2);

        // Rightward sweep
        REQUIRE(lb[0].loopType() == LoopType::DOUBLE_SWEEP);
        REQUIRE(lb[0].iterationDeltaType() == LoopIterationDeltaType::IRREGULAR);
        REQUIRE(lb[0].minDpDelta() == -1);
        REQUIRE(!lb[0].maxDpDelta()); // Irregular growth
        REQUIRE(!lb[0].endDpGrowth());

        // Leftward sweep
        REQUIRE(lb[1].loopType() == LoopType::DOUBLE_SWEEP);
        REQUIRE(lb[1].iterationDeltaType() == LoopIterationDeltaType::IRREGULAR);
        REQUIRE(lb[1].minDpDelta() == -1);
        REQUIRE(!lb[1].maxDpDelta()); // Irregular growth
        REQUIRE(lb[1].endDpGrowth() == 1);

        result = checker.init(&mla, hangExecutor);
        REQUIRE(result);

        REQUIRE(!checker.isIrregular(DataDirection::LEFT));
        REQUIRE(checker.isIrregular(DataDirection::RIGHT));
        REQUIRE(checker.insweepExit(DataDirection::RIGHT) == 1);
        REQUIRE(checker.insweepToggle(DataDirection::RIGHT) == 2);
    }

    SECTION("BasicIrregularSweepAtLeft") {
        // Irregular sweep appendix at left side with values -2 and -1. The first -2 exits the
        // sweep and toggles to -1. All swept -1's toggle to -2 on sweep return.
        // Leftward sweep loop (exits on zero or minus two)
        block[0].finalize(MOV, -1, dummySteps, block + 4, block + 1);
        block[1].finalize(INC,  2, dummySteps, block + 3, block + 2);
        block[2].finalize(INC, -2, dummySteps, exitBlock, block + 0);

        // Exit on minus two
        block[3].finalize(INC, -1, dummySteps, exitBlock, block + 5);

        // Exit on zero (extends sweep)
        block[4].finalize(INC, -2, dummySteps, exitBlock, block + 5);

        // Rightward sweep loop (toggle all -1's to -2's)
        block[5].finalize(MOV,  1, dummySteps, block + 7, block + 6);
        block[6].finalize(INC, -1, dummySteps, exitBlock, block + 5);

        // Transition at left (extends sweep)
        block[7].finalize(INC, -3, dummySteps, exitBlock, block + 0);

        auto program = std::make_shared<InterpretedProgramFromArray>(block, maxSequenceLen);
        hangExecutor.execute(program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        REQUIRE(result);
        REQUIRE(mla.metaLoopType() == MetaLoopType::IRREGULAR);
        REQUIRE(mla.loopSize() == 4);

        auto lb = mla.loopBehaviors();
        REQUIRE(lb.size() == 2);

        // Leftward sweep
        REQUIRE(lb[0].loopType() == LoopType::DOUBLE_SWEEP);
        REQUIRE(lb[0].iterationDeltaType() == LoopIterationDeltaType::IRREGULAR);
        REQUIRE(!lb[0].minDpDelta()); // Irregular growth
        REQUIRE(lb[0].maxDpDelta() == 1);
        REQUIRE(!lb[0].endDpGrowth());

        // Leftward sweep
        REQUIRE(lb[1].loopType() == LoopType::DOUBLE_SWEEP);
        REQUIRE(lb[1].iterationDeltaType() == LoopIterationDeltaType::IRREGULAR);
        REQUIRE(!lb[1].minDpDelta()); // Irregular growth
        REQUIRE(lb[1].maxDpDelta() == 1);
        REQUIRE(lb[1].endDpGrowth() == 1);

        result = checker.init(&mla, hangExecutor);
        REQUIRE(result);

        REQUIRE(checker.isIrregular(DataDirection::LEFT));
        REQUIRE(!checker.isIrregular(DataDirection::RIGHT));
        REQUIRE(checker.insweepExit(DataDirection::LEFT) == -2);
        REQUIRE(checker.insweepToggle(DataDirection::LEFT) == -1);
    }

    SECTION("BodylessIrregularSweep") {
        // Rightward sweep loop (exits on zero or one)
        block[0].finalize(MOV,  1, dummySteps, block + 4, block + 1);
        block[1].finalize(INC, -1, dummySteps, block + 3, block + 2);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 0);

        // Exit on one
        block[3].finalize(INC,  2, dummySteps, exitBlock, block + 5);

        // Exit on zero (extends sweep)
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);

        // Leftward sweep loop (toggle all twos to ones)
        block[5].finalize(MOV, -1, dummySteps, block + 0, block + 6);
        block[6].finalize(INC, -1, dummySteps, exitBlock, block + 5);

        auto program = std::make_shared<InterpretedProgramFromArray>(block, maxSequenceLen);
        hangExecutor.execute(program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        REQUIRE(result);

        result = checker.init(&mla, hangExecutor);
        REQUIRE(result);

        REQUIRE(!checker.isIrregular(DataDirection::LEFT));
        REQUIRE(checker.isIrregular(DataDirection::RIGHT));
        REQUIRE(checker.insweepExit(DataDirection::RIGHT) == 1);
        REQUIRE(checker.insweepToggle(DataDirection::RIGHT) == 2);
    }

    SECTION("BodylessIrregularSweep-2") {
        // Sweep does not have a plain body, only an appendix.
        // Furthermore, the leftward sweep varies per exit

        // Rightward sweep loop (exits on zero or one)
        block[0].finalize(MOV,  1, dummySteps, block + 3, block + 1);
        block[1].finalize(INC, -1, dummySteps, block + 4, block + 2);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 0);

        // Exit on zero
        block[3].finalize(INC,  1, dummySteps, exitBlock, block + 5);

        // Exit on one
        block[4].finalize(INC,  2, dummySteps, exitBlock, block + 6);

        // Leftward sweep loop (only move)
        block[5].finalize(MOV, -1, dummySteps, block + 0, block + 5);

        // Leftward sweep loop (toggle all twos to ones)
        block[6].finalize(MOV, -1, dummySteps, block + 0, block + 7);
        block[7].finalize(INC, -1, dummySteps, exitBlock, block + 6);

        auto program = std::make_shared<InterpretedProgramFromArray>(block, maxSequenceLen);
        hangExecutor.execute(program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        REQUIRE(result);

        REQUIRE(mla.metaLoopType() == MetaLoopType::IRREGULAR);
        REQUIRE(mla.loopSize() == 4);

        auto lb = mla.loopBehaviors();
        REQUIRE(lb.size() == 2);

        // Leftward sweep
        REQUIRE(lb[0].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[0].iterationDeltaType() == LoopIterationDeltaType::IRREGULAR);
        REQUIRE(lb[0].minDpDelta() == 0);
        REQUIRE(!lb[0].maxDpDelta()); // Irregular growth
        REQUIRE(lb[0].endDpGrowth() == 0);

        // Rightward sweep
        REQUIRE(lb[1].loopType() == LoopType::ANCHORED_SWEEP);
        REQUIRE(lb[1].iterationDeltaType() == LoopIterationDeltaType::IRREGULAR);
        REQUIRE(lb[1].minDpDelta() == 0);
        REQUIRE(!lb[1].maxDpDelta()); // Irregular growth
        REQUIRE(!lb[1].endDpGrowth());

        result = checker.init(&mla, hangExecutor);
        REQUIRE(result);

        REQUIRE(!checker.isIrregular(DataDirection::LEFT));
        REQUIRE(checker.isIrregular(DataDirection::RIGHT));
        REQUIRE(checker.insweepExit(DataDirection::RIGHT) == 1);
        REQUIRE(checker.insweepToggle(DataDirection::RIGHT) == 2);
    }

    SECTION("IrregularSweepWithMidSweepTransition") {
        // Right: Rightward sweep loop (exits on zero or one)
        block[0].finalize(MOV,  1, dummySteps, block + 4, block + 1);
        block[1].finalize(INC, -1, dummySteps, block + 3, block + 2);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 0);

        // Exit on one
        block[3].finalize(INC,  2, dummySteps, exitBlock, block + 5);

        // Exit on zero (extends sweep)
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);

        // Right: Leftward sweep loop (toggle all twos to ones)
        block[5].finalize(MOV, -1, dummySteps, block + 7, block + 6);
        block[6].finalize(INC, -1, dummySteps, exitBlock, block + 5);

        // Left: Leftward sweep loop (plain)
        block[7].finalize(MOV, -1, dummySteps, block + 8, block + 7);

        // Extension at left
        block[8].finalize(INC,  1, dummySteps, exitBlock, block + 9);

        // Left: Rightward sweep
        block[9].finalize(MOV,  1, dummySteps, block + 0, block + 9);

        auto program = std::make_shared<InterpretedProgramFromArray>(block, maxSequenceLen);
        hangExecutor.execute(program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        REQUIRE(result);

        REQUIRE(mla.metaLoopType() == MetaLoopType::IRREGULAR);
        REQUIRE(mla.loopSize() == 6);

        auto lb = mla.loopBehaviors();
        REQUIRE(lb.size() == 4);

        result = checker.init(&mla, hangExecutor);
        REQUIRE(result);

        REQUIRE(!checker.isIrregular(DataDirection::LEFT));
        REQUIRE(checker.isIrregular(DataDirection::RIGHT));
        REQUIRE(checker.insweepExit(DataDirection::RIGHT) == 1);
        REQUIRE(checker.insweepToggle(DataDirection::RIGHT) == 2);
    }
}
