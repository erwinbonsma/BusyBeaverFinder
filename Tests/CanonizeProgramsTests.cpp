//
//  CanonizeProgramsTests.cpp
//  Tests
//
//  Created by Erwin on 02/03/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "Utils.h"
#include "InterpretedProgram.h"
#include "InterpretedProgramCanonizer.h"
#include "ProgramBlock.h"

constexpr int dummySteps = 1;
constexpr int maxSequenceLen = 16;

constexpr bool INC = true;
constexpr bool MOV = false;

TEST_CASE("Canonize programs test", "[program][canonize]") {
    auto program = create_indexed_array<ProgramBlock, maxSequenceLen>();
    ProgramBlock *block = program.data();
    ProgramBlock *exitBlock = &block[maxSequenceLen - 1];
    exitBlock->finalizeExit(dummySteps);

    SECTION("SimpleProgram") {
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 4);
        block[4].finalize(INC,  4, dummySteps, block + 3, block + 2);
        block[3].finalize(INC,  3, dummySteps, block + 4, block + 2);
        block[2].finalize(INC, -2, dummySteps, exitBlock, block + 1);
        block[1].finalize(INC,  3, dummySteps, block + 4, block + 2); // Same as Block 3

        InterpretedProgramFromArray  sourceProgram {block, maxSequenceLen};
        InterpretedProgramCanonizer  canonical {sourceProgram};

//        sourceProgram.dump();
//        canonical.dump();
//        canonical.dumpCanonicalProgram(std::cout);

        REQUIRE(canonical.canonicalProgramString() == "0+1_12 1EXIT 2+4_34 3+3_24 4-2_13");
    }
}
