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

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxSteps = 1024;
    settings.hangSamplePeriod = 32;
    searcher.configure(settings);

    orchestrator.search();

    REQUIRE(tracker.getMaxStepsFound() == 43);
    // There are still two hangs that are not detected.
    REQUIRE(tracker.getTotalHangs() - tracker.getTotalEarlyHangs() <= 2);
}
