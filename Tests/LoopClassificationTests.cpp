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

    LoopClassification classification;

    SECTION( "StationarySingleChange" ) {
        loopBlock[0].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 0);

        classification.classifyLoop(loopBlock, 1);

        REQUIRE(classification.dataPointerDelta() == 0);
        REQUIRE(classification.numDataDeltas() == 1);
    }
    SECTION( "StationaryTwoChanges" ) {
        loopBlock[0].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(false, 1, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 3);
        loopBlock[3].finalize(false, -1, dummySteps, &exitBlock, loopBlock + 0);

        classification.classifyLoop(loopBlock, 4);
        classification.dump();

        REQUIRE(classification.dataPointerDelta() == 0);
        REQUIRE(classification.numDataDeltas() == 2);
    }
    SECTION( "StationaryOscillating" ) {
        loopBlock[0].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 0);

        classification.classifyLoop(loopBlock, 2);

        REQUIRE(classification.dataPointerDelta() == 0);
        REQUIRE(classification.numDataDeltas() == 0);
    }
    SECTION( "TravellingConstant" ) {
        loopBlock[0].finalize(false, 1, dummySteps, &exitBlock, loopBlock + 0);

        classification.classifyLoop(loopBlock, 1);

        REQUIRE(classification.dataPointerDelta() == 1);
        REQUIRE(classification.numDataDeltas() == 0);
    }
    SECTION( "TravellingSingleChange" ) {
        loopBlock[0].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(false, 1, dummySteps, &exitBlock, loopBlock + 0);

        classification.classifyLoop(loopBlock, 2);
        classification.dump();

        REQUIRE(classification.dataPointerDelta() == 1);
        REQUIRE(classification.numDataDeltas() == 1);
    }
    SECTION( "TravellingTwoChanges" ) {
        loopBlock[0].finalize(false, 1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(false, 1, dummySteps, &exitBlock, loopBlock + 3);
        loopBlock[3].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 0);

        classification.classifyLoop(loopBlock, 4);
        classification.dump();

        REQUIRE(classification.dataPointerDelta() == 2);
        REQUIRE(classification.numDataDeltas() == 2);
    }
    SECTION( "TravellingTwoChanges2" ) {
        // A variant of TravellingOscillating2 but with modified shifts so that deltas do not
        // cancel each other out
        loopBlock[0].finalize(false, -1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(false, 2, dummySteps, &exitBlock, loopBlock + 3);
        loopBlock[3].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 4);
        loopBlock[4].finalize(false, 2, dummySteps, &exitBlock, loopBlock + 0);

        classification.classifyLoop(loopBlock, 5);
        classification.dump();

        REQUIRE(classification.dataPointerDelta() == 3);
        REQUIRE(classification.numDataDeltas() == 2);
    }
    SECTION( "TravellingOscillating" ) {
        loopBlock[0].finalize(false, 2, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(false, -1, dummySteps, &exitBlock, loopBlock + 3);
        loopBlock[3].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 0);

        classification.classifyLoop(loopBlock, 4);
        classification.dump();

        REQUIRE(classification.dataPointerDelta() == 1);
        REQUIRE(classification.numDataDeltas() == 0);
    }
    SECTION( "TravellingOscillating2" ) {
        // A more complex oscillating change, where original DP-offsets are both positive and
        // negative, which requires sign-aware modulus matching to collapse these entries.
        loopBlock[0].finalize(false, -1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(true, -1, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(false, 3, dummySteps, &exitBlock, loopBlock + 3);
        loopBlock[3].finalize(true, 1, dummySteps, &exitBlock, loopBlock + 4);
        loopBlock[4].finalize(false, 1, dummySteps, &exitBlock, loopBlock + 0);

        classification.classifyLoop(loopBlock, 5);
        classification.dump();

        REQUIRE(classification.dataPointerDelta() == 3);
        REQUIRE(classification.numDataDeltas() == 0);
    }
}
