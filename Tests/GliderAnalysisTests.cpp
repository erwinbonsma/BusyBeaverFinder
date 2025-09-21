//
//  GliderAnalysisTests.cpp
//  Tests
//
//  Created by Erwin on 16/12/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#include "catch.hpp"

#include "GliderHangChecker.h"
#include "MetaLoopAnalysis.h"
#include "ProgramBlock.h"
#include "RunUntilMetaLoop.h"

const int dummySteps = 1;
const int maxSequenceLen = 16;

const bool INC = true;
const bool MOV = false;

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

        auto program = std::make_shared<InterpretedProgramFromArray>(block, maxSequenceLen);
        hangExecutor.execute(program);

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

        auto program = std::make_shared<InterpretedProgramFromArray>(block, maxSequenceLen);
        hangExecutor.execute(program);

        bool result = mla.analyzeMetaLoop(hangExecutor);
        auto lb = mla.loopBehaviors();

        REQUIRE(result);
        REQUIRE(mla.loopSize() == 2);
        REQUIRE(lb.size() == 1);

        // Glider loop
        REQUIRE(lb[0].loopType() == LoopType::GLIDER);
        REQUIRE(lb[0].iterationDeltaType() == LoopIterationDeltaType::NONLINEAR_INCREASE);
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

        auto program = std::make_shared<InterpretedProgramFromArray>(block, maxSequenceLen);
        hangExecutor.execute(program);

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

        auto program = std::make_shared<InterpretedProgramFromArray>(block, maxSequenceLen);
        hangExecutor.execute(program);

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
