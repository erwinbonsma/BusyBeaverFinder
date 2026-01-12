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

TEST_CASE("5x5 OrchestratedSearch", "[search][5x5][orchestrated]") {
    SearchSettings settings {5};
    OrchestratedSearchRunner runner {settings};

    auto tracker = std::make_unique<ProgressTracker>();
    tracker->setDumpSuccessStepsLimit(INT_MAX);
    runner.getSearcher().attachProgressTracker(std::move(tracker));

    SECTION("Find all") {
        runner.run();

        tracker = runner.getSearcher().detachProgressTracker();
        REQUIRE(tracker->getMaxStepsFound() == 44);
        REQUIRE(tracker->getTotalSuccess() == 26319);
        REQUIRE(tracker->getTotalDetectedHangs() == 4228);
        REQUIRE(tracker->getTotalHangs() == 4228);
        REQUIRE(tracker->getTotalErrors() == 0);
    }
}

TEST_CASE("6x6 OrchestratedSearch", "[search][6x6][orchestrated][.explicit]") {
    SearchSettings settings {6};
    settings.dataSize = 6000;
    settings.maxHangDetectionSteps = 50000;
    settings.maxSearchSteps =  50000;
    settings.maxSteps = 100000;
    OrchestratedSearchRunner runner {settings};

    auto tracker = std::make_unique<ProgressTracker>();
    tracker->setDumpUndetectedHangs(true);
    tracker->setDumpStatsPeriod(10000000);
    tracker->setDumpStackPeriod(10000000);
    runner.getSearcher().attachProgressTracker(std::move(tracker));

    SECTION("Find all") {
        runner.run();

        tracker = runner.getSearcher().detachProgressTracker();
        tracker->dumpFinalStats();

        REQUIRE(tracker->getMaxStepsFound() == 573);
        REQUIRE(tracker->getTotalSuccess() == 6475715);
        REQUIRE(tracker->getTotalHangs() == 1546939);
        REQUIRE(tracker->getTotalErrors() == 0);
        REQUIRE(tracker->getTotalDetectedHangs() == 1546935);
    }
}
