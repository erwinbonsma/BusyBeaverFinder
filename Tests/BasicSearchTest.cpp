//
//  BasicSearchTest.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 03/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>
#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "3x3 Search", "[search]" ) {
    ExhaustiveSearcher *searcher = new ExhaustiveSearcher(3, 3, 16);
    ProgressTracker *tracker = new ProgressTracker(searcher);

    searcher->setProgressTracker(tracker);

    SECTION( "Find all" ) {
        searcher->search();

        REQUIRE(tracker->getMaxStepsFound() == 4);
        REQUIRE(tracker->getTotalSuccess() == 59);
    }
    SECTION( "Find one" ) {
        searcher->findOne();

        REQUIRE(tracker->getMaxStepsFound() == 3);
        REQUIRE(tracker->getTotalSuccess() == 1);
    }

    delete searcher;
    delete tracker;
}

TEST_CASE( "4x4 Search", "[search]" ) {
    ExhaustiveSearcher *searcher = new ExhaustiveSearcher(4, 4, 32);
    ProgressTracker *tracker = new ProgressTracker(searcher);

    searcher->setProgressTracker(tracker);
    searcher->setMaxStepsPerRun(1024);
    searcher->setHangSamplePeriod(32);

    searcher->search();

    REQUIRE(tracker->getMaxStepsFound() == 14);
    REQUIRE(tracker->getTotalSuccess() == 854);
    REQUIRE(tracker->getTotalEarlyHangs() == tracker->getTotalHangs());

    delete searcher;
    delete tracker;
}


TEST_CASE( "5x5 Search", "[search]" ) {
    ExhaustiveSearcher *searcher = new ExhaustiveSearcher(5, 5, 128);
    ProgressTracker *tracker = new ProgressTracker(searcher);

    searcher->setProgressTracker(tracker);
    searcher->setMaxStepsPerRun(1024);
    searcher->setHangSamplePeriod(32);

    searcher->search();

    REQUIRE(tracker->getMaxStepsFound() == 43);
    REQUIRE(tracker->getTotalSuccess() == 51410);
    // There are still two hangs that are not detected.
    REQUIRE(tracker->getTotalHangs() - tracker->getTotalEarlyHangs() <= 2);

    delete searcher;
    delete tracker;
}
