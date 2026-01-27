//
//  SequenceAnalysisTests.cpp
//  Tests
//
//  Created by Erwin on 13/11/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "Utils.h"
#include "SequenceAnalysis.h"
#include "ProgramBlock.h"

constexpr int dummySteps = 1;
constexpr int maxSequenceLen = 4;

constexpr bool INC = true;
constexpr bool MOV = false;
constexpr bool EQ = true;
constexpr bool NEQ = false;

void analyzeSequence(SequenceAnalysis& sa, ProgramBlock* startBlock, int numBlocks) {
    const ProgramBlock* blockP[maxSequenceLen];

    for (int i = 0; i < numBlocks; ++i) {
        blockP[i] = startBlock++;
    }

    sa.analyzeSequence(blockP, numBlocks);
}

TEST_CASE( "Pre-condition tests", "[sequence-analysis]" ) {
    ProgramBlock exitBlock(-1);

    auto program = create_indexed_array<ProgramBlock, maxSequenceLen>();
    ProgramBlock *block = program.data();

    SequenceAnalysis sa;

    SECTION( "NoPreCondition" ) {
        block[0].finalize(INC, 1, dummySteps, &exitBlock, block + 0);

        analyzeSequence(sa, block, 1);

        REQUIRE(sa.preConditions().empty());
    }
    SECTION( "PreConditionUnequal" ) {
        block[0].finalize(INC, 3, dummySteps, &exitBlock, block + 1);
        block[1].finalize(MOV, 2, dummySteps, &exitBlock, &exitBlock);

        analyzeSequence(sa, block, 2);

        REQUIRE(sa.preConditions().count(0) == 1);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-3, NEQ)));
    }
    SECTION( "PreConditionEquals" ) {
        block[0].finalize(INC, 3, dummySteps, block + 1, &exitBlock);
        block[1].finalize(MOV, 2, dummySteps, &exitBlock, &exitBlock);

        analyzeSequence(sa, block, 2);

        REQUIRE(sa.preConditions().count(0) == 1);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-3, EQ)));
    }
    SECTION( "MultipleUnequalPreConditions" ) {
        block[0].finalize(INC, 3, dummySteps, &exitBlock, block + 1);
        block[1].finalize(INC, 2, dummySteps, &exitBlock, block + 2);
        block[2].finalize(MOV, 1, dummySteps, &exitBlock, &exitBlock);

        analyzeSequence(sa, block, 3);

        REQUIRE(sa.preConditions().count(0) == 2);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-3, NEQ)));
        REQUIRE(sa.hasPreCondition(0, PreCondition(-5, NEQ)));
    }
    SECTION( "EqualConditionCancelsEarlierUnequalPreCondition" ) {
        block[0].finalize(INC, 3, dummySteps, &exitBlock, block + 1);
        block[1].finalize(INC, 2, dummySteps, block + 2, &exitBlock);
        block[2].finalize(MOV, 1, dummySteps, &exitBlock, &exitBlock);

        analyzeSequence(sa, block, 3);

        REQUIRE(sa.preConditions().count(0) == 1);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-5, EQ)));
    }
    SECTION( "EqualConditionSurpressesLaterUnequalPreCondition" ) {
        block[0].finalize(INC, 3, dummySteps, block + 1, &exitBlock);
        block[1].finalize(INC, 2, dummySteps, &exitBlock, block + 2);
        block[2].finalize(MOV, 1, dummySteps, &exitBlock, &exitBlock);

        analyzeSequence(sa, block, 3);

        REQUIRE(sa.preConditions().count(0) == 1);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-3, EQ)));
    }
    SECTION( "PreConditionsDoNotRepeat" ) {
        block[0].finalize(INC, 1, dummySteps, &exitBlock, block + 1);
        block[1].finalize(INC, -1, dummySteps, &exitBlock, block + 2);
        block[2].finalize(INC, 1, dummySteps, &exitBlock, block + 3);
        block[3].finalize(MOV, 1, dummySteps, &exitBlock, &exitBlock);

        analyzeSequence(sa, block, 4);

        REQUIRE(sa.preConditions().count(0) == 2);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-1, NEQ)));
        REQUIRE(sa.hasPreCondition(0, PreCondition(0, NEQ)));
    }
    SECTION( "PreConditionsForDifferentOffsetsAreIndependent" ) {
        block[0].finalize(INC, 3, dummySteps, &exitBlock, block + 1);
        block[1].finalize(MOV, 1, dummySteps, &exitBlock, block + 2);
        block[2].finalize(INC, 2, dummySteps, &exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, &exitBlock, &exitBlock);

        analyzeSequence(sa, block,4);

        REQUIRE(sa.preConditions().count(0) == 1);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-3, NEQ)));

        REQUIRE(sa.preConditions().count(1) == 2);
        REQUIRE(sa.hasPreCondition(1, PreCondition(0, NEQ)));
        REQUIRE(sa.hasPreCondition(1, PreCondition(-2, NEQ)));
    }
}
