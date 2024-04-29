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
#include "SweepHangChecker.h"

const int dummySteps = 1;
const int maxSequenceLen = 16;

const bool INC = true;
const bool MOV = false;

// Sweep programs that may or may not hang.
TEST_CASE("Hang analysis (irregular sweeps)", "[hang-analysis][sweep][irregular]") {
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

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

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

        REQUIRE(!checker.leftIsIrregular());
        REQUIRE(checker.rightIsIrregular());
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

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        REQUIRE(result);

        result = checker.init(&mla, hangExecutor);
        REQUIRE(result);

        REQUIRE(!checker.leftIsIrregular());
        REQUIRE(checker.rightIsIrregular());
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

        InterpretedProgramFromArray program(block, maxSequenceLen);
        hangExecutor.execute(&program);

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

        REQUIRE(!checker.leftIsIrregular());
        REQUIRE(checker.rightIsIrregular());
    }
}
