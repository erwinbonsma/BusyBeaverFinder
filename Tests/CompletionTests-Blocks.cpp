//
//  CompletionTests-Blocks.cpp
//  Tests
//
//  Created by Erwin on 27/02/2024.
//  Copyright © 2024 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "FastExecutor.h"
#include "InterpretedProgram.h"
#include "ProgramBlock.h"
//#include "HangExecutor.h"

const int dummySteps = 1;
const int maxSequenceLen = 16;

const bool INC = true;
const bool MOV = false;

TEST_CASE("Block-based Completion Tests", "[success][blocks][.explicit]") {
    FastExecutor hangExecutor(1024);
    hangExecutor.setMaxSteps(1000000000);

    ProgramBlock block[maxSequenceLen];
    for (int i = 0; i < maxSequenceLen; i++) {
        block[i].init(i);
    }
    ProgramBlock *exitBlock = &block[maxSequenceLen - 1];
    exitBlock->finalizeExit(dummySteps);

    SECTION("VeryLongRunningDoubleGliderWithSweep") {
        // Based on "GliderWithWake-PowersOfThree"
        //
        // At the right is a glider which leaves values f(n) = 3^n - 1 in its wake. When the
        // glider finishes a meta-iteration, it carries out a sweep which subtracts 7 from each
        // value. When the sweep returns to the glider, another glider meta-iteration is executed,
        // generating the next power of three. The program terminates when subtracting seven
        // results in zero. This happens for f(n) = f(6) = 728 after 104 sweeps. This means that
        // the glider loop meanwhile generates f(n) = f(6 + 104) > 10^52 which takes the same order
        // of run block executions.

        // Bootstrap: Now = 1
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 2, exitBlock);

        // Loop: Wake += 3, Now -= 1, Next += 3
        block[2].finalize(INC,  3, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -2, dummySteps, block + 4, block + 5);
        block[4].finalize(INC,  3, dummySteps, exitBlock, block + 6);
        block[5].finalize(INC,  3, dummySteps, exitBlock, block + 6);
        block[6].finalize(MOV,  1, dummySteps, exitBlock, block + 7);
        block[7].finalize(INC, -1, dummySteps, block + 9, block + 8);
        block[8].finalize(MOV,  1, dummySteps, exitBlock, block + 2);

        // Transition
        block[ 9].finalize(MOV, -1, dummySteps, exitBlock, block + 10);
        block[10].finalize(INC, -1, dummySteps, exitBlock, block + 11);

        // Left sweep
        block[11].finalize(MOV, -1, dummySteps, block + 13, block + 12);
        block[12].finalize(INC, -7, dummySteps, exitBlock,  block + 11); // Program exit

        // Right sweep
        block[13].finalize(MOV, 1, dummySteps, block + 14, block + 13);

        // Transition into glider
        block[14].finalize(MOV, 2, dummySteps, block + 2, exitBlock);

        auto program = std::make_shared<InterpretedProgramFromArray>(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(program);
        hangExecutor.dump();

        // This program does not actually hang, but detecting completion is out of scope.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
}
