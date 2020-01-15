//
//  LoopClassificationTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include <stdio.h>

#include "catch.hpp"

#include "LoopClassification.h"
#include "ProgramBlock.h"

const int dummySteps = 1;

TEST_CASE( "Loop classification tests", "[classify-loop]" ) {
    ProgramBlock exitBlock;
    exitBlock.init(-1);

    ProgramBlock loopBlock[maxLoopSize];
    for (int i = 0; i < maxLoopSize; i++) {
        loopBlock[i].init(i);
    }

    LoopClassification lc;

    SECTION( "StationarySingleChangeIncByOne" ) {
        loopBlock[0].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 0);

        lc.classifyLoop(loopBlock, 1);
        lc.dump();

        REQUIRE(lc.dataPointerDelta() == 0);
        REQUIRE(lc.numDataDeltas() == 1);

        REQUIRE(lc.exit(0).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -1));
    }
    SECTION( "StationarySingleChangeDecByThree" ) {
        loopBlock[0].finalize(true, -3, dummySteps, &exitBlock, loopBlock + 0);

        lc.classifyLoop(loopBlock, 1);
        lc.dump();

        REQUIRE(lc.dataPointerDelta() == 0);
        REQUIRE(lc.numDataDeltas() == 1);

        REQUIRE(lc.exit(0).exitCondition.expressionEquals(Operator::GREATER_THAN_OR_EQUAL, 3));
        REQUIRE(lc.exit(0).exitCondition.modulusContraintEquals(-3));

        REQUIRE(lc.exit(0).exitCondition.isTrueForValue(3));
        REQUIRE(lc.exit(0).exitCondition.isTrueForValue(6));
        REQUIRE(lc.exit(0).exitCondition.isTrueForValue(9));
        REQUIRE(!lc.exit(0).exitCondition.isTrueForValue(0));
        REQUIRE(!lc.exit(0).exitCondition.isTrueForValue(2));
        REQUIRE(!lc.exit(0).exitCondition.isTrueForValue(4));
    }
    SECTION( "StationarySingleChangeInTwoSteps" ) {
        loopBlock[0].finalize(true, 2, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(true, 3, dummySteps, &exitBlock, loopBlock + 0);

        lc.classifyLoop(loopBlock, 2);
        lc.dump();

        REQUIRE(lc.dataPointerDelta() == 0);
        REQUIRE(lc.numDataDeltas() == 1);

        REQUIRE(lc.exit(0).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -2));
        REQUIRE(lc.exit(0).exitCondition.modulusContraintEquals(5));
        REQUIRE(lc.exit(1).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -5));
        REQUIRE(lc.exit(1).exitCondition.modulusContraintEquals(5));

        REQUIRE(lc.exit(0).exitCondition.isTrueForValue(-2));
        REQUIRE(lc.exit(0).exitCondition.isTrueForValue(-7));
        REQUIRE(lc.exit(0).exitCondition.isTrueForValue(-12));
        REQUIRE(!lc.exit(0).exitCondition.isTrueForValue(-3));
        REQUIRE(!lc.exit(0).exitCondition.isTrueForValue(3));
        REQUIRE(!lc.exit(0).exitCondition.isTrueForValue(0));
    }
    SECTION( "StationarySingleChangeInThreeSteps" ) {
        loopBlock[0].finalize(true, 5, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(true, -2, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(true, 2, dummySteps, &exitBlock, loopBlock + 0);

        lc.classifyLoop(loopBlock, 3);
        lc.dump();

        REQUIRE(lc.dataPointerDelta() == 0);
        REQUIRE(lc.numDataDeltas() == 1);

        REQUIRE(lc.exit(0).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -5));
        REQUIRE(lc.exit(0).exitCondition.modulusContraintEquals(5));
        REQUIRE(lc.exit(1).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -3));
        REQUIRE(lc.exit(1).exitCondition.modulusContraintEquals(5));
        REQUIRE(lc.exit(2).bootstrapOnly); // Cannot be reached.
    }
    SECTION( "StationaryTwoChanges" ) {
        loopBlock[0].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(false, 1, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(true, -2, dummySteps, &exitBlock, loopBlock + 3);
        loopBlock[3].finalize(false, -1, dummySteps, &exitBlock, loopBlock + 0);

        lc.classifyLoop(loopBlock, 4);
        lc.dump();

        REQUIRE(lc.dataPointerDelta() == 0);
        REQUIRE(lc.numDataDeltas() == 2);

        REQUIRE(lc.exit(0).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -1));
        REQUIRE(lc.exit(0).exitCondition.modulusContraintEquals(1));
        REQUIRE(!lc.exit(0).bootstrapOnly);
        REQUIRE(lc.exit(1).exitCondition.expressionEquals(Operator::GREATER_THAN_OR_EQUAL, 0));
        REQUIRE(lc.exit(1).exitCondition.modulusContraintEquals(-2));
        REQUIRE(lc.exit(1).bootstrapOnly);
        REQUIRE(lc.exit(2).exitCondition.expressionEquals(Operator::GREATER_THAN_OR_EQUAL, 2));
        REQUIRE(lc.exit(2).exitCondition.modulusContraintEquals(-2));
        REQUIRE(!lc.exit(2).bootstrapOnly);
        REQUIRE(lc.exit(3).bootstrapOnly);
    }
    SECTION( "StationaryOscillating" ) {
        loopBlock[0].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 0);

        lc.classifyLoop(loopBlock, 2);

        REQUIRE(lc.dataPointerDelta() == 0);
        REQUIRE(lc.numDataDeltas() == 0);
    }
    SECTION( "TravellingConstant" ) {
        loopBlock[0].finalize(false, 1, dummySteps, &exitBlock, loopBlock + 0);

        lc.classifyLoop(loopBlock, 1);

        REQUIRE(lc.dataPointerDelta() == 1);
        REQUIRE(lc.numDataDeltas() == 0);
    }
    SECTION( "TravellingSingleChange" ) {
        loopBlock[0].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(false, 1, dummySteps, &exitBlock, loopBlock + 0);

        lc.classifyLoop(loopBlock, 2);
        lc.dump();

        REQUIRE(lc.dataPointerDelta() == 1);
        REQUIRE(lc.numDataDeltas() == 1);
    }
    SECTION( "TravellingTwoChanges" ) {
        loopBlock[0].finalize(false, 1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(false, 1, dummySteps, &exitBlock, loopBlock + 3);
        loopBlock[3].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 0);

        lc.classifyLoop(loopBlock, 4);
        lc.dump();

        REQUIRE(lc.dataPointerDelta() == 2);
        REQUIRE(lc.numDataDeltas() == 2);
    }
    SECTION( "TravellingTwoChanges2" ) {
        // A variant of TravellingOscillating2 but with modified shifts so that deltas do not
        // cancel each other out
        loopBlock[0].finalize(false, -1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(false, 2, dummySteps, &exitBlock, loopBlock + 3);
        loopBlock[3].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 4);
        loopBlock[4].finalize(false, 2, dummySteps, &exitBlock, loopBlock + 0);

        lc.classifyLoop(loopBlock, 5);
        lc.dump();

        REQUIRE(lc.dataPointerDelta() == 3);
        REQUIRE(lc.numDataDeltas() == 2);
    }
    SECTION( "TravellingOscillating" ) {
        loopBlock[0].finalize(false, 2, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(false, -1, dummySteps, &exitBlock, loopBlock + 3);
        loopBlock[3].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 0);

        lc.classifyLoop(loopBlock, 4);
        lc.dump();

        REQUIRE(lc.dataPointerDelta() == 1);
        REQUIRE(lc.numDataDeltas() == 0);
    }
    SECTION( "TravellingOscillating2" ) {
        // A more complex oscillating change, where original DP-offsets are both positive and
        // negative, which requires sign-aware modulus matching to collapse these entries.
        loopBlock[0].finalize(false, -1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(false, 3, dummySteps, &exitBlock, loopBlock + 3);
        loopBlock[3].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 4);
        loopBlock[4].finalize(false, 1, dummySteps, &exitBlock, loopBlock + 0);

        lc.classifyLoop(loopBlock, 5);
        lc.dump();

        REQUIRE(lc.dataPointerDelta() == 3);
        REQUIRE(lc.numDataDeltas() == 0);
    }
}
