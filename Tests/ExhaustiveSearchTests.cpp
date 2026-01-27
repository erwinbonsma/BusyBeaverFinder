//
//  ExhaustiveSearchTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 03/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <memory>
#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "3x3 Search", "[search][3x3][exhaustive]" ) {
    SearchSettings settings {3};
    ExhaustiveSearcher searcher {settings};

    auto tracker = std::make_unique<ProgressTracker>();
    tracker->setDumpSuccessStepsLimit(INT_MAX);
    searcher.attachProgressTracker(std::move(tracker));

    SECTION("3x3-FindAll") {
        searcher.search();

        tracker = searcher.detachProgressTracker();
        REQUIRE(tracker->getMaxStepsFound() == 5);
        REQUIRE(tracker->getTotalSuccess() == 59);
    }
    SECTION("3x3-FindOne") {
        searcher.findOne();

        tracker = searcher.detachProgressTracker();
        REQUIRE(tracker->getMaxStepsFound() == 4);
        REQUIRE(tracker->getTotalSuccess() == 1);
    }
}

TEST_CASE( "4x4 Search", "[search][4x4][exhaustive]" ) {
    SearchSettings settings {4};
    ExhaustiveSearcher searcher {settings};

    auto tracker = std::make_unique<ProgressTracker>();
    tracker->setDumpSuccessStepsLimit(INT_MAX);
    searcher.attachProgressTracker(std::move(tracker));

    SECTION("Find all") {
        searcher.search();

        tracker = searcher.detachProgressTracker();
        REQUIRE(tracker->getMaxStepsFound() == 15);
        REQUIRE(tracker->getTotalSuccess() == 854);
        REQUIRE(tracker->getTotalErrors() == 0);
        REQUIRE(tracker->getTotalDetectedHangs() == tracker->getTotalHangs());
    }
}

TEST_CASE( "5x5 Search", "[search][5x5][exhaustive]" ) {
    SearchSettings settings {5};
    ExhaustiveSearcher searcher {settings};

    auto tracker = std::make_unique<ProgressTracker>();
    tracker->setDumpSuccessStepsLimit(INT_MAX);
    tracker->setDumpUndetectedHangs(true);
    searcher.attachProgressTracker(std::move(tracker));

    SECTION("Find all") {
        searcher.search();

        tracker = searcher.detachProgressTracker();
        REQUIRE(tracker->getMaxStepsFound() == 44);
        REQUIRE(tracker->getTotalSuccess() == 51410);
        REQUIRE(tracker->getTotalErrors() == 0);
        REQUIRE(tracker->getTotalHangs() == tracker->getTotalDetectedHangs());
    }
}
