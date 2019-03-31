//
//  ExhaustiveSearchTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 03/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>
#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "3x3 Search", "[search][3x3][exhaustive]" ) {
    ExhaustiveSearcher searcher(3, 3, 16);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    searcher.setProgressTracker(&tracker);

    SECTION( "Find all" ) {
        searcher.search();

        REQUIRE(tracker.getMaxStepsFound() == 4);
        REQUIRE(tracker.getTotalSuccess() == 59);
    }
    SECTION( "Find one" ) {
        searcher.findOne();

        REQUIRE(tracker.getMaxStepsFound() == 3);
        REQUIRE(tracker.getTotalSuccess() == 1);
    }
}

TEST_CASE( "4x4 Search", "[search][4x4][exhaustive]" ) {
    ExhaustiveSearcher searcher(4, 4, 32);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxSteps = 1024;
    searcher.configure(settings);

    searcher.search();

    REQUIRE(tracker.getMaxStepsFound() == 14);
    REQUIRE(tracker.getTotalSuccess() == 854);
    REQUIRE(tracker.getTotalErrors() == 0);
    REQUIRE(tracker.getTotalDetectedHangs() == tracker.getTotalHangs());
}

TEST_CASE( "5x5 Search", "[search][5x5][exhaustive]" ) {
    ExhaustiveSearcher searcher(5, 5, 128);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    tracker.setDumpUndetectedHangs(true);
    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxSteps = 2048;
    searcher.configure(settings);

    searcher.search();

    REQUIRE(tracker.getMaxStepsFound() == 43);
    REQUIRE(tracker.getTotalSuccess() == 51410);
    REQUIRE(tracker.getTotalErrors() == 0);
    REQUIRE(tracker.getTotalHangs() == tracker.getTotalDetectedHangs());
}
