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

        const RunBlock* runBlock = runSummary.runBlockAt(runBlockIndex);

        if (expectedSequenceIndex == numSequences) {
            // Expect a new sequence. Check that the sequence was indeed not yet encountered.
            for (int i = 0; i < numSequences; i++) {
                if (sequenceIndex[i] == runBlock->getSequenceIndex()) {
                    std::cout << "Expected a new sequence but found an existing one" << std::endl;
                    return false;
                }
            }
            // Map the actual ID of the encountered sequence to the normalized expected ID
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

void populateRunSummary(RunSummary& runSummary, ProgramBlockIndex* programBlocks) {
    ProgramBlockIndex* programBlockP = programBlocks;
    while (*programBlockP >= 0) {
        runSummary.recordProgramBlock(*programBlockP++);
    }
    runSummary.dump();
}

TEST_CASE( "RunSummary", "[util][runsummary]" ) {
    RunSummary runSummary;
    int zArrayHelperBuf[32];
    runSummary.setCapacity(64, zArrayHelperBuf);

    SECTION( "SingleBlockRunBlocks" ) {
        // Each run block consists of a single program block
        ProgramBlockIndex blocks[] = {1, 3,3,3, 2, 5,5,5, 4, 3,3,3, 2, 5,5,5, 4, 3,3,3, -1};
        populateRunSummary(runSummary, blocks);

        int expectedRuns[] {0,1, 1,3, 2,1, 3,3, 4,1, 1,3, 2,1, 3,3, 4,1, 1,3, -1};
        REQUIRE(checkRunSummary(runSummary, expectedRuns));
    }
    SECTION( "SingleBlockRunBlocks2" ) {
        // Same as previous, but starting with a loop
        ProgramBlockIndex blocks[] = {1,1,1, 0, 3,3,3, 2, 1,1,1, 0, 3,3,3, 2, 1,1,1, -1};
        populateRunSummary(runSummary, blocks);

        int expectedRuns[] {0,3, 1,1, 2,3, 3,1, 0,3, 1,1, 2,3, 3,1, 0,3, -1};
        REQUIRE(checkRunSummary(runSummary, expectedRuns));
    }
    SECTION( "MultiBlockLoop" ) {
        // Execution contains loop that consists of more than one program block
        ProgramBlockIndex blocks[] = {1, 3, 5, 7, 5, 7, 4, 5, 7, 5, 7, 4, -1};
        populateRunSummary(runSummary, blocks);

        int expectedRuns[] {0,2, 1,4, 2,1, 1,4, -1};
        REQUIRE(checkRunSummary(runSummary, expectedRuns));
    }
    SECTION( "MultiBlockLoopWithPrematureExit" ) {
        // Same as previous, but loop is aborted prematurely
        ProgramBlockIndex blocks[] = {1, 3, 5, 7, 5, 7, 5, 6, 5, 7, 5, 7, 5, 6, -1};
        populateRunSummary(runSummary, blocks);

        int expectedRuns[] {0,2, 1,5, 2,1, 1,5, -1};
        REQUIRE(checkRunSummary(runSummary, expectedRuns));
    }
    SECTION( "MultiBlockSwitch" ) {
        // Execution contains a switch between loops that consists of more than one program block
        ProgramBlockIndex blocks[] = {1, 3, 5, 5, 5, 4, 7, 9, 3, 5, 5, 5, 4, 7, 9, 3, 5, 5, 5, -1};
        populateRunSummary(runSummary, blocks);

        int expectedRuns[] {0,2, 1,3, 2,4, 1,3, 2,4, 1,3, -1};
        REQUIRE(checkRunSummary(runSummary, expectedRuns));
    }
    SECTION( "MultiBlockSwitches" ) {
        // Execution contains multiple multi-block switches
        ProgramBlockIndex blocks[] = {1, 3, 5, 5, 4, 7, 5, 5, 4, 6, 9, 5, 5, -1};
        populateRunSummary(runSummary, blocks);

        int expectedRuns[] {0,2, 1,2, 2,2, 1,2, 3,3, 1,2, -1};
        REQUIRE(checkRunSummary(runSummary, expectedRuns));
    }
    SECTION( "ManyLoopExits" ) {
        // Loop that has three different exits
        ProgramBlockIndex blocks[] = {1,1, 2, 1,1, 3, 1,1, 4, 1,1, 2, 1,1, 4, 1,1, 3, 1,1, -1};
        populateRunSummary(runSummary, blocks);

        int expectedRuns[] {0,2, 1,1, 0,2, 2,1, 0,2, 3,1, 0,2, 1,1, 0,2, 3,1, 0,2, 2,1, 0,2, -1};
        REQUIRE(checkRunSummary(runSummary, expectedRuns));
    }
    SECTION( "LoopWithRepeatedProgramBlocks" ) {
        // Loop that contains some repeated run blocks
        ProgramBlockIndex blocks[] = {1, 2,3,2,4,2,3,2,4, 5, 2,3,2,4,2,3,2,4, 5, -1};
        populateRunSummary(runSummary, blocks);

        int expectedRuns[] {0,1, 1,8, 2,1, 1,8, -1};
        REQUIRE(checkRunSummary(runSummary, expectedRuns));
    }
    SECTION( "DifferentSequencesWithEqualEndings" ) {
        // This program contains different sequences that end with the same program block. These
        // should result in different run blocks, but they were not distinguished by an earlier
        // implementation of RunSummary (before 4fdc8fb commit)
        ProgramBlockIndex blocks[] = {1,2,3, 4,4, 1,5,3, 4,4, 1,2,3, 4,4, 1,5,3, 4,4, -1};
        populateRunSummary(runSummary, blocks);

        int expectedRuns[] {0,3, 1,2, 2,3, 1,2, 0,3, 1,2, 2,3, 1,2, -1};
        REQUIRE(checkRunSummary(runSummary, expectedRuns));
    }
}

TEST_CASE( "RunSummaryLoopEquivalence", "[util][runsummary][loop-equivalence" ) {
    RunSummary runSummary;
    int zArrayHelperBuf[32];
    runSummary.setCapacity(64, zArrayHelperBuf);

    SECTION( "EqualThreeBlockLoops" ) {
        ProgramBlockIndex blocks[] = {1, 2,3,4,2,3,4, 1, 3,4,2,3,4,2, 1, 4,2,3,4,2,3, -1};
        populateRunSummary(runSummary, blocks);

        REQUIRE(runSummary.getNumRunBlocks() == 6);
        auto loop1 = runSummary.runBlockAt(1);
        auto loop2 = runSummary.runBlockAt(3);
        auto loop3 = runSummary.runBlockAt(5);
        REQUIRE(loop1->isLoop());
        REQUIRE(loop2->isLoop());
        REQUIRE(loop3->isLoop());

        REQUIRE(runSummary.areLoopsRotationEqual(loop1, loop2));
        REQUIRE(runSummary.areLoopsRotationEqual(loop1, loop3));
        REQUIRE(runSummary.areLoopsRotationEqual(loop2, loop3));
        REQUIRE(runSummary.areLoopsRotationEqual(loop2, loop1));
        REQUIRE(runSummary.areLoopsRotationEqual(loop3, loop1));
        REQUIRE(runSummary.areLoopsRotationEqual(loop3, loop2));
    }
    SECTION( "UnequalThreeBlockLoops" ) {
        ProgramBlockIndex blocks[] = {1, 2,3,4,2,3,4, 1, 3,5,2,3,5,2, 1, 4,2,6,4,2,6, -1};
        populateRunSummary(runSummary, blocks);

        REQUIRE(runSummary.getNumRunBlocks() == 6);
        auto loop1 = runSummary.runBlockAt(1);
        auto loop2 = runSummary.runBlockAt(3);
        auto loop3 = runSummary.runBlockAt(5);
        REQUIRE(loop1->isLoop());
        REQUIRE(loop2->isLoop());
        REQUIRE(loop3->isLoop());

        REQUIRE(!runSummary.areLoopsRotationEqual(loop1, loop2));
        REQUIRE(!runSummary.areLoopsRotationEqual(loop1, loop3));
        REQUIRE(!runSummary.areLoopsRotationEqual(loop2, loop3));
        REQUIRE(!runSummary.areLoopsRotationEqual(loop2, loop1));
        REQUIRE(!runSummary.areLoopsRotationEqual(loop3, loop1));
        REQUIRE(!runSummary.areLoopsRotationEqual(loop3, loop2));
    }
}
