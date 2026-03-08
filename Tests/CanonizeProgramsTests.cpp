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

    SECTION("CanonizingReordersInstructions") {
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 4); // a
        block[4].finalize(MOV,  4, dummySteps, block + 3, block + 2); // c
        block[3].finalize(INC,  3, dummySteps, nullptr,   block + 2); // d
        block[2].finalize(INC, -2, dummySteps, exitBlock, block + 1); // e
        block[1].finalize(MOV, -3, dummySteps, block + 4, block + 2); // f

        InterpretedProgramFromArray  sourceProgram {block, maxSequenceLen};
        InterpretedProgramCanonizer  canonical {sourceProgram};

        std::string expected {"a+1bc bX c>4de d+3-e e-2bf f<3ce"};
        REQUIRE(canonical.canonicalProgramString() == expected);
    }
    SECTION("CanonizingCombinesExits") {
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1); // a
        block[1].finalize(MOV, -3, dummySteps, block + 2, exitBlock); // c
        block[2].finalize(INC, -2, dummySteps, block + 3, block + 4); // d
        block[3].finalize(MOV,  1, dummySteps, exitBlock, block + 2); // e
        block[4].finalize(MOV,  2, dummySteps, block + 1, block + 2); // f

        InterpretedProgramFromArray  sourceProgram {block, maxSequenceLen};
        InterpretedProgramCanonizer  canonical {sourceProgram};

        std::string expected {"a+1bc bX c<3db d-2ef e>1bd f>2cd"};
        REQUIRE(canonical.canonicalProgramString() == expected);
    }
    SECTION("CanonizingCombinesInstructions") {
        block[0].finalize(MOV,  1, dummySteps, block + 1, block + 2); // a
        block[1].finalize(MOV,  1, dummySteps, block + 2, block + 3); // b
        block[2].finalize(MOV,  1, dummySteps, block + 1, block + 2); // a
        block[3].finalize(MOV,  2, dummySteps, block + 1, block + 2); // c
        block[4].finalize(MOV,  1, dummySteps, block + 1, block + 0); // a

        InterpretedProgramFromArray  sourceProgram {block, maxSequenceLen};
        InterpretedProgramCanonizer  canonical {sourceProgram};

        std::string expected {"a>1ba b>1ac c>2ba"};
        REQUIRE(canonical.canonicalProgramString() == expected);
    }
}
