//
//  RunSummaryTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 07/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>
#include <iostream>

#include "catch.hpp"

#include "RunSummary.h"

const int maxNumSequences = 16;

bool checkRunSummary(RunSummary& runSummary, int* expected) {
    int* expectedP = expected;
    int runBlockIndex = 0;
    int numSequences = 0;
    int sequenceIndex[maxNumSequences];

    while (*expectedP >= 0) {
        if (runBlockIndex == runSummary.getNumRunBlocks()) {
            std::cout << "Run summary too short" << std::endl;
            return false;
        }

        int expectedSequenceIndex = *expectedP++; // Normalized
        int expectedSequenceLength = *expectedP++;

        RunBlock* runBlock = runSummary.runBlockAt(runBlockIndex);

        if (expectedSequenceIndex == numSequences) {
            sequenceIndex[numSequences++] = runBlock->getSequenceIndex();
        } else {
            if (sequenceIndex[expectedSequenceIndex] != runBlock->getSequenceIndex()) {
                std::cout
                    << "Unexpected sequence index for Run #" << runBlockIndex << std::endl;
                return false;
            }
        }

        if (expectedSequenceLength != runSummary.getRunBlockLength(runBlockIndex)) {
            std::cout
                << "Unexpected sequence length for Run #" << runBlockIndex << std::endl;
            return false;
        }

        runBlockIndex++;
    }

    return true;
}

bool executeRunSummaryTest(ProgramBlockIndex* programBlocks, int* expectedRuns) {
    RunSummary runSummary;

    runSummary.setCapacity(64);

    runSummary.reset();

    ProgramBlockIndex* programBlockP = programBlocks;
    while (*programBlockP >= 0) {
        runSummary.recordProgramBlock(*programBlockP++);
    }
    runSummary.dump();

    return checkRunSummary(runSummary, expectedRuns);
}

TEST_CASE( "RunSummary", "[util][runsummary]" ) {
    SECTION( "SingleBlockRunBlocks" ) {
        // Each run block consists of a single program block
        ProgramBlockIndex blocks[] = {1, 3,3,3, 2, 5,5,5, 4, 3,3,3, 2, 5,5,5, 4, 3,3,3, -1};
        int expected[] {0,1, 1,3, 2,1, 3,3, 4,1, 1,3, 2,1, 3,3, 4,1, 1,3, -1};

        REQUIRE(executeRunSummaryTest(blocks, expected));
    }
    SECTION( "SingleBlockRunBlocks2" ) {
        // Same as previous, but starting with a loop
        ProgramBlockIndex blocks[] = {1,1,1, 0, 3,3,3, 2, 1,1,1, 0, 3,3,3, 2, 1,1,1, -1};
        int expected[] {0,3, 1,1, 2,3, 3,1, 0,3, 1,1, 2,3, 3,1, 0,3, -1};

        REQUIRE(executeRunSummaryTest(blocks, expected));
    }
    SECTION( "MultiBlockLoop" ) {
        // Execution contains loop that consists of more than one program block
        ProgramBlockIndex blocks[] = {1, 3, 5, 7, 5, 7, 4, 5, 7, 5, 7, 4, -1};
        int expected[] {0,2, 1,4, 2,1, 1,4, -1};

        REQUIRE(executeRunSummaryTest(blocks, expected));
    }
    SECTION( "MultiBlockLoopWithPrematureExit" ) {
        // Same as previous, but loop is aborted prematurely
        ProgramBlockIndex blocks[] = {1, 3, 5, 7, 5, 6, 5, 7, 5, 7, 5, 6, -1};
        int expected[] {0,2, 1,3, 2,1, 1,5, -1};

        REQUIRE(executeRunSummaryTest(blocks, expected));
    }
    SECTION( "MultiBlockSwitch" ) {
        // Execution contains a switch between loops that consists of more than one program block
        ProgramBlockIndex blocks[] = {1, 3, 5, 5, 5, 4, 7, 9, 3, 5, 5, 5, 4, 7, 9, 3, 5, 5, 5, -1};
        int expected[] {0,2, 1,3, 2,4, 1,3, 2,4, 1,3, -1};

        REQUIRE(executeRunSummaryTest(blocks, expected));
    }
    SECTION( "MultiBlockSwitches" ) {
        // Execution contains multiple multi-block switches
        ProgramBlockIndex blocks[] = {1, 3, 5, 5, 4, 7, 5, 5, 4, 6, 9, 5, 5, -1};
        int expected[] {0,2, 1,2, 2,2, 1,2, 3,3, 1,2, -1};

        REQUIRE(executeRunSummaryTest(blocks, expected));
    }
}
