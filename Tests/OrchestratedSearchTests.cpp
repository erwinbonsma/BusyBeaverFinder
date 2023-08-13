//
//  OrchestratedSearchTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 11/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>
#include "catch.hpp"

#include "ExhaustiveSearcher.h"
#include "SearchOrchestration.h"

TEST_CASE( "5x5 OrchestratedSearch", "[search][5x5][orchestrated]" ) {
    SearchSettings settings = defaultSearchSettings;
    settings.maxSteps = 2048;

    ExhaustiveSearcher searcher(5, 5, settings);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    searcher.setProgressTracker(&tracker);

    SECTION("Find all") {
        orchestratedSearch(searcher);
        //    tracker.dumpFinalStats();

        REQUIRE(tracker.getMaxStepsFound() == 44);
        REQUIRE(tracker.getTotalSuccess() == 26319);
        REQUIRE(tracker.getTotalDetectedHangs() == 4228);
        REQUIRE(tracker.getTotalHangs() == 4228);
        REQUIRE(tracker.getTotalErrors() == 0);
    }
}

TEST_CASE( "6x6 OrchestratedSearch", "[search][6x6][orchestrated][.explicit]" ) {
    SearchSettings settings = defaultSearchSettings;
    settings.dataSize = 4096;
    settings.maxHangDetectionSteps = 20000;
    settings.maxSteps = 100000;
    settings.undoCapacity = settings.maxSteps;
//    settings.testHangDetection = true;

    ExhaustiveSearcher searcher(6, 6, settings);
    ProgressTracker tracker(searcher);

    tracker.setDumpUndetectedHangs(true);
    tracker.setDumpStatsPeriod(10000000);
    tracker.setDumpStackPeriod(10000000);
    searcher.setProgressTracker(&tracker);

    SECTION ("Find all") {
        orchestratedSearch(searcher);
        tracker.dumpFinalStats();

        REQUIRE(tracker.getMaxStepsFound() == 573);
        REQUIRE(tracker.getTotalSuccess() == 6475715);
        REQUIRE(tracker.getTotalHangs() == 1546939);
        REQUIRE(tracker.getTotalErrors() == 0);
        REQUIRE(tracker.getTotalDetectedHangs() == 1546895);
    }
}
