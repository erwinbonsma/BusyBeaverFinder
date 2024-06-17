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
    SearchSettings settings {};
    ExhaustiveSearcher searcher(3, 3, settings);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    searcher.setProgressTracker(&tracker);

    SECTION( "Find all" ) {
        searcher.search();

        REQUIRE(tracker.getMaxStepsFound() == 5);
        REQUIRE(tracker.getTotalSuccess() == 59);
    }
    SECTION( "Find one" ) {
        searcher.findOne();

        REQUIRE(tracker.getMaxStepsFound() == 4);
        REQUIRE(tracker.getTotalSuccess() == 1);
    }
}

TEST_CASE( "4x4 Search", "[search][4x4][exhaustive]" ) {
    SearchSettings settings {};
    ExhaustiveSearcher searcher(4, 4, settings);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    searcher.setProgressTracker(&tracker);

    SECTION("Find all") {
        searcher.search();

        REQUIRE(tracker.getMaxStepsFound() == 15);
        REQUIRE(tracker.getTotalSuccess() == 854);
        REQUIRE(tracker.getTotalErrors() == 0);
        REQUIRE(tracker.getTotalDetectedHangs() == tracker.getTotalHangs());
    }
}

TEST_CASE( "5x5 Search", "[search][5x5][exhaustive]" ) {
    SearchSettings settings {};
    ExhaustiveSearcher searcher(5, 5, settings);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    tracker.setDumpUndetectedHangs(true);
    searcher.setProgressTracker(&tracker);

    SECTION("Find all") {
        searcher.search();

        REQUIRE(tracker.getMaxStepsFound() == 44);
        REQUIRE(tracker.getTotalSuccess() == 51410);
        REQUIRE(tracker.getTotalErrors() == 0);
        REQUIRE(tracker.getTotalHangs() == tracker.getTotalDetectedHangs());
    }
}
