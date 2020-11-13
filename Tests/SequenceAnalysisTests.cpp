//
//  SequenceAnalysisTests.cpp
//  Tests
//
//  Created by Erwin on 13/11/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "SequenceAnalysis.h"
#include "ProgramBlock.h"

const int dummySteps = 1;
const int maxSequenceLen = 16;

const int INC = true;
const int MOV = false;
const bool EQ = true;
const bool NEQ = false;

TEST_CASE( "Pre-condition tests", "[sequence-analysis]" ) {
    ProgramBlock exitBlock;
    exitBlock.init(-1);

    ProgramBlock seqBlock[maxSequenceLen];
    for (int i = 0; i < maxSequenceLen; i++) {
        seqBlock[i].init(i);
    }

    SequenceAnalysis sa;

    SECTION( "NoPreCondition" ) {
        seqBlock[0].finalize(INC, 1, dummySteps, &exitBlock, seqBlock + 0);

        sa.analyzeSequence(seqBlock, 1);

        REQUIRE(sa.preConditions().empty());
    }
    SECTION( "PreConditionUnequal" ) {
        seqBlock[0].finalize(INC, 3, dummySteps, &exitBlock, seqBlock + 1);
        seqBlock[1].finalize(MOV, 2, dummySteps, &exitBlock, &exitBlock);

        sa.analyzeSequence(seqBlock, 2);

        REQUIRE(sa.preConditions().count(0) == 1);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-3, NEQ)));
    }
    SECTION( "PreConditionEquals" ) {
        seqBlock[0].finalize(INC, 3, dummySteps, seqBlock + 1, &exitBlock);
        seqBlock[1].finalize(MOV, 2, dummySteps, &exitBlock, &exitBlock);

        sa.analyzeSequence(seqBlock, 2);

        REQUIRE(sa.preConditions().count(0) == 1);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-3, EQ)));
    }
    SECTION( "MultipleUnequalPreConditions" ) {
        seqBlock[0].finalize(INC, 3, dummySteps, &exitBlock, seqBlock + 1);
        seqBlock[1].finalize(INC, 2, dummySteps, &exitBlock, seqBlock + 2);
        seqBlock[2].finalize(MOV, 1, dummySteps, &exitBlock, &exitBlock);

        sa.analyzeSequence(seqBlock, 3);

        REQUIRE(sa.preConditions().count(0) == 2);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-3, NEQ)));
        REQUIRE(sa.hasPreCondition(0, PreCondition(-5, NEQ)));
    }
    SECTION( "EqualConditionCancelsEarlierUnequalPreCondition" ) {
        seqBlock[0].finalize(INC, 3, dummySteps, &exitBlock, seqBlock + 1);
        seqBlock[1].finalize(INC, 2, dummySteps, seqBlock + 2, &exitBlock);
        seqBlock[2].finalize(MOV, 1, dummySteps, &exitBlock, &exitBlock);

        sa.analyzeSequence(seqBlock, 3);

        REQUIRE(sa.preConditions().count(0) == 1);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-5, EQ)));
    }
    SECTION( "EqualConditionSurpressesLaterUnequalPreCondition" ) {
        seqBlock[0].finalize(INC, 3, dummySteps, seqBlock + 1, &exitBlock);
        seqBlock[1].finalize(INC, 2, dummySteps, &exitBlock, seqBlock + 2);
        seqBlock[2].finalize(MOV, 1, dummySteps, &exitBlock, &exitBlock);

        sa.analyzeSequence(seqBlock, 3);

        REQUIRE(sa.preConditions().count(0) == 1);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-3, EQ)));
    }
    SECTION( "PreConditionsDoNotRepeat" ) {
        seqBlock[0].finalize(INC, 1, dummySteps, &exitBlock, seqBlock + 1);
        seqBlock[1].finalize(INC, -1, dummySteps, &exitBlock, seqBlock + 2);
        seqBlock[2].finalize(INC, 1, dummySteps, &exitBlock, seqBlock + 3);
        seqBlock[3].finalize(MOV, 1, dummySteps, &exitBlock, &exitBlock);

        sa.analyzeSequence(seqBlock, 4);

        REQUIRE(sa.preConditions().count(0) == 2);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-1, NEQ)));
        REQUIRE(sa.hasPreCondition(0, PreCondition(0, NEQ)));
    }
    SECTION( "PreConditionsForDifferentOffsetsAreIndependent" ) {
        seqBlock[0].finalize(INC, 3, dummySteps, &exitBlock, seqBlock + 1);
        seqBlock[1].finalize(MOV, 1, dummySteps, &exitBlock, seqBlock + 2);
        seqBlock[2].finalize(INC, 2, dummySteps, &exitBlock, seqBlock + 3);
        seqBlock[3].finalize(MOV, -1, dummySteps, &exitBlock, &exitBlock);

        sa.analyzeSequence(seqBlock, 4);

        REQUIRE(sa.preConditions().count(0) == 1);
        REQUIRE(sa.hasPreCondition(0, PreCondition(-3, NEQ)));

        REQUIRE(sa.preConditions().count(1) == 2);
        REQUIRE(sa.hasPreCondition(1, PreCondition(0, NEQ)));
        REQUIRE(sa.hasPreCondition(1, PreCondition(-2, NEQ)));
    }
}
