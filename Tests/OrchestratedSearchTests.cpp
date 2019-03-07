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
    settings.maxSteps = 1024;
    searcher.configure(settings);

    orchestrator.search();

    REQUIRE(tracker.getMaxStepsFound() == 43);
    REQUIRE(tracker.getTotalSuccess() == 26319);
    REQUIRE(tracker.getTotalEarlyHangs() == 4228);
    REQUIRE(tracker.getTotalHangs() == 4228);
    REQUIRE(tracker.getTotalErrors() == 0);
}

TEST_CASE( "6x6 OrchestratedSearch", "[search][6x6][orchestrated][.explicit]" ) {
    ExhaustiveSearcher searcher(6, 6, 1024);
    ProgressTracker tracker(searcher);
    SearchOrchestrator orchestrator(searcher);

//    tracker.setDumpUndetectedHangs(true);
    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxSteps = 1024;
    searcher.configure(settings);

    orchestrator.search();
    tracker.dumpFinalStats();

    REQUIRE(tracker.getMaxStepsFound() == 572);
    REQUIRE(tracker.getTotalSuccess() == 6475715);
    REQUIRE(tracker.getTotalEarlyHangs() == 1543614);
    REQUIRE(tracker.getTotalHangs() == 1546939);
    REQUIRE(tracker.getTotalErrors() == 0);
}
