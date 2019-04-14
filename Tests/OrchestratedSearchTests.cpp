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
#include "SearchOrchestrator.h"

TEST_CASE( "5x5 OrchestratedSearch", "[search][5x5][orchestrated]" ) {
    ExhaustiveSearcher searcher(5, 5, 128);
    ProgressTracker tracker(searcher);
    SearchOrchestrator orchestrator(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxSteps = 2048;
    searcher.configure(settings);

    orchestrator.search();
//    tracker.dumpFinalStats();

    REQUIRE(tracker.getMaxStepsFound() == 43);
    REQUIRE(tracker.getTotalSuccess() == 26319);
    REQUIRE(tracker.getTotalDetectedHangs() == 4228);
    REQUIRE(tracker.getTotalHangs() == 4228);
    REQUIRE(tracker.getTotalErrors() == 0);
}

TEST_CASE( "6x6 OrchestratedSearch", "[search][6x6][orchestrated][.explicit]" ) {
    ExhaustiveSearcher searcher(6, 6, 4096);
    ProgressTracker tracker(searcher);
    SearchOrchestrator orchestrator(searcher);

//    tracker.setDumpUndetectedHangs(true);
//    tracker.setDumpStatsPeriod(10000000);
//    tracker.setDumpStackPeriod(10000000);
    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxSteps = 16384;
    settings.maxPeriodicHangDetectAttempts = 6;
//    settings.testHangDetection = true;
    searcher.configure(settings);

    orchestrator.search();
    tracker.dumpFinalStats();

    REQUIRE(tracker.getMaxStepsFound() == 572);
    REQUIRE(tracker.getTotalSuccess() == 6475715);
    REQUIRE(tracker.getTotalDetectedHangs() == 1542528);
//    REQUIRE(tracker.getTotalHangs() == 1546939);
//    REQUIRE(tracker.getTotalErrors() == 0);
    // TEMP
    REQUIRE((tracker.getTotalHangs() + tracker.getTotalErrors()) == 1546939);
}
