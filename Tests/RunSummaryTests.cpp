//
//  RunSummaryTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 07/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>
#include <iostream>
#include <set>
#include <vector>

#include "catch.hpp"

#include "RunSummary.h"

class RunSummaryTest : public RunSummaryBase {
    const std::vector<int> &_runHistory;

    int getRunUnitIdAt(int runUnitIndex) const override { return _runHistory[runUnitIndex]; };

public:
    RunSummaryTest(const std::vector<int> &runHistory) : _runHistory(runHistory) {}

    bool processNewRunUnits() override { return processNewHistory(_runHistory); };
};

bool checkRunSummary(RunSummaryTest& runSummary, std::vector<int>& expected) {
    auto expectedIt = expected.begin();
    int runBlockIndex = 0;
    // Maps run summary sequence IDs to expected IDs
    std::map<int, int> idMap;

    while (expectedIt != expected.end()) {
        if (runBlockIndex == runSummary.getNumRunBlocks()) {
            std::cout << "Run summary too short" << std::endl;
            return false;
        }

        int expectedSequenceIndex = *expectedIt++; // Normalized
        int expectedSequenceLength = *expectedIt++;

        const RunBlock* runBlock = runSummary.runBlockAt(runBlockIndex);

        if (expectedSequenceIndex == idMap.size()) {
            // Expect a new sequence. Check that the sequence was indeed not yet encountered.

            if (idMap.find(runBlock->getSequenceId()) != idMap.end()) {
                std::cout << "Expected a new sequence but found an existing one" << std::endl;
                return false;
            }

            idMap.insert({runBlock->getSequenceId(), expectedSequenceIndex});
        } else {
            auto result = idMap.find(runBlock->getSequenceId());
            if (result == idMap.end() || result->second != expectedSequenceIndex) {
                std::cout << "Unexpected sequence index for Run #" << runBlockIndex << std::endl;
                return false;
            }
        }

        if (expectedSequenceLength != runSummary.getRunBlockLength(runBlockIndex)) {
            std::cout << "Unexpected sequence length for Run #" << runBlockIndex << std::endl;
            return false;
        }

        runBlockIndex++;
    }

    return true;
}

bool processAndCompare(std::vector<int>& blocks, std::vector<int>& expectedRuns) {
    RunSummaryTest runSummary(blocks);
    int zArrayHelperBuf[32];

    runSummary.setHelperBuffer(zArrayHelperBuf);
    runSummary.processNewRunUnits();
    runSummary.dump();

    return checkRunSummary(runSummary, expectedRuns);
}

TEST_CASE( "RunSummary", "[util][runsummary]" ) {
    SECTION( "SingleBlockRunBlocks" ) {
        // Each run block consists of a single program block
        std::vector<int> v = {1, 3,3,3, 2, 5,5,5, 4, 3,3,3, 2, 5,5,5, 4, 3,3,3};
        std::vector<int> expectedRuns = {0,1, 1,3, 2,1, 3,3, 4,1, 1,3, 2,1, 3,3, 4,1, 1,3};

        REQUIRE(processAndCompare(v, expectedRuns));
    }
    SECTION( "SingleBlockRunBlocks2" ) {
        // Same as previous, but starting with a loop
        std::vector<int> v = {1,1,1, 0, 3,3,3, 2, 1,1,1, 0, 3,3,3, 2, 1,1,1};
        std::vector<int> expectedRuns = {0,3, 1,1, 2,3, 3,1, 0,3, 1,1, 2,3, 3,1, 0,3};

        REQUIRE(processAndCompare(v, expectedRuns));
    }
    SECTION( "MultiBlockLoop" ) {
        // Execution contains loop that consists of more than one program block
        std::vector<int> v = {1,3, 5,7,5,7, 4, 5,7,5,7, 4};
        std::vector<int> expectedRuns = {0,2, 1,4, 2,1, 1,4};

        REQUIRE(processAndCompare(v, expectedRuns));
    }
    SECTION( "MultiBlockLoopWithPrematureExit" ) {
        // Same as previous, but loop is aborted prematurely
        std::vector<int> v = {1,3, 5,7,5,7,5, 6, 5,7,5,7,5, 6};
        std::vector<int> expectedRuns = {0,2, 1,5, 2,1, 1,5};

        REQUIRE(processAndCompare(v, expectedRuns));
    }
    SECTION( "MultiBlockSwitch" ) {
        // Execution contains a switch between loops that consists of more than one program block
        std::vector<int> v = {1,3, 5,5,5, 4,7,9,3, 5,5,5, 4,7,9,3, 5,5,5};
        std::vector<int> expectedRuns = {0,2, 1,3, 2,4, 1,3, 2,4, 1,3};

        REQUIRE(processAndCompare(v, expectedRuns));
    }
    SECTION( "MultiBlockSwitches" ) {
        // Execution contains multiple multi-block switches
        std::vector<int> v = {1,3, 5,5, 4,7, 5,5, 4,6,9, 5,5};
        std::vector<int> expectedRuns = {0,2, 1,2, 2,2, 1,2, 3,3, 1,2};

        REQUIRE(processAndCompare(v, expectedRuns));
    }
    SECTION( "ManyLoopExits" ) {
        // Loop that has three different exits
        std::vector<int> v = {1,1, 2, 1,1, 3, 1,1, 4, 1,1, 2, 1,1, 4, 1,1, 3, 1,1};
        std::vector<int> expectedRuns =
            {0,2, 1,1, 0,2, 2,1, 0,2, 3,1, 0,2, 1,1, 0,2, 3,1, 0,2, 2,1, 0,2};

        REQUIRE(processAndCompare(v, expectedRuns));
    }
    SECTION( "LoopWithRepeatedProgramBlocks" ) {
        // Loop that contains some repeated run blocks
        std::vector<int> v = {1, 2,3,2,4,2,3,2,4, 5, 2,3,2,4,2,3,2,4, 5};
        std::vector<int> expectedRuns = {0,1, 1,8, 2,1, 1,8};

        REQUIRE(processAndCompare(v, expectedRuns));
    }
    SECTION( "DifferentSequencesWithEqualEndings" ) {
        // This program contains different sequences that end with the same program block. These
        // should result in different run blocks, but they were not distinguished by an earlier
        // implementation of RunSummary (before 4fdc8fb commit)
        std::vector<int> v = {1,2,3, 4,4, 1,5,3, 4,4, 1,2,3, 4,4, 1,5,3, 4,4};
        std::vector<int> expectedRuns = {0,3, 1,2, 2,3, 1,2, 0,3, 1,2, 2,3, 1,2};

        REQUIRE(processAndCompare(v, expectedRuns));
    }
}

TEST_CASE( "RunSummaryLoopEquivalence", "[util][runsummary][loop-equivalence]" ) {
    std::vector<int> history;
    RunSummaryTest runSummary(history);
    int zArrayHelperBuf[32];
    runSummary.setHelperBuffer(zArrayHelperBuf);
    int loopOffset;

    SECTION( "EqualThreeBlockLoops" ) {
        std::vector<int> v = {1, 2,3,4,2,3,4, 1, 3,4,2,3,4,2, 1, 4,2,3,4,2,3};
        std::swap(history, v);
        runSummary.processNewRunUnits();

        REQUIRE(runSummary.getNumRunBlocks() == 6);
        auto loop1 = runSummary.runBlockAt(1);
        auto loop2 = runSummary.runBlockAt(3);
        auto loop3 = runSummary.runBlockAt(5);
        REQUIRE(loop1->isLoop());
        REQUIRE(loop2->isLoop());
        REQUIRE(loop3->isLoop());

        REQUIRE(runSummary.areLoopsRotationEqual(loop1, loop2, loopOffset));
        REQUIRE(loopOffset == 1);
        REQUIRE(runSummary.areLoopsRotationEqual(loop1, loop3, loopOffset));
        REQUIRE(loopOffset == 2);
        REQUIRE(runSummary.areLoopsRotationEqual(loop2, loop3, loopOffset));
        REQUIRE(loopOffset == 1);
        REQUIRE(runSummary.areLoopsRotationEqual(loop2, loop1, loopOffset));
        REQUIRE(loopOffset == 2);
        REQUIRE(runSummary.areLoopsRotationEqual(loop3, loop1, loopOffset));
        REQUIRE(loopOffset == 1);
        REQUIRE(runSummary.areLoopsRotationEqual(loop3, loop2, loopOffset));
        REQUIRE(loopOffset == 2);
    }
    SECTION( "UnequalThreeBlockLoops" ) {
        std::vector<int> v = {1, 2,3,4,2,3,4, 1, 3,5,2,3,5,2, 1, 4,2,6,4,2,6};
        std::swap(history, v);
        runSummary.processNewRunUnits();

        REQUIRE(runSummary.getNumRunBlocks() == 6);
        auto loop1 = runSummary.runBlockAt(1);
        auto loop2 = runSummary.runBlockAt(3);
        auto loop3 = runSummary.runBlockAt(5);
        REQUIRE(loop1->isLoop());
        REQUIRE(loop2->isLoop());
        REQUIRE(loop3->isLoop());

        REQUIRE(!runSummary.areLoopsRotationEqual(loop1, loop2, loopOffset));
        REQUIRE(!runSummary.areLoopsRotationEqual(loop1, loop3, loopOffset));
        REQUIRE(!runSummary.areLoopsRotationEqual(loop2, loop3, loopOffset));
        REQUIRE(!runSummary.areLoopsRotationEqual(loop2, loop1, loopOffset));
        REQUIRE(!runSummary.areLoopsRotationEqual(loop3, loop1, loopOffset));
        REQUIRE(!runSummary.areLoopsRotationEqual(loop3, loop2, loopOffset));
    }
}
