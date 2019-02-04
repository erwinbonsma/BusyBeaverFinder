//
//  HangTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 03/02/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#include <stdio.h>
#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "5x5 Hang tests", "[hang][5x5]" ) {
    ExhaustiveSearcher *searcher = new ExhaustiveSearcher(5, 5, 64);
    ProgressTracker *tracker = new ProgressTracker(searcher);

    searcher->setProgressTracker(tracker);
    searcher->setHangSamplePeriod(16);

    SECTION( "BasicLoop" ) {
        // *   *
        // o . . . *
        // . * . .
        // .     *
        Op resumeFrom[] = {
            Op::NOOP, Op::NOOP, Op::DATA, Op::TURN,
            Op::NOOP, Op::NOOP, Op::NOOP, Op::TURN,
            Op::NOOP, Op::TURN,
            Op::NOOP, Op::TURN,
            Op::TURN,
            Op::UNSET
        };
        searcher->findOne(resumeFrom);

        REQUIRE(tracker->getTotalEarlyHangs() == 1);
    }
    SECTION( "CountingLoop" ) {
        // *   *
        // o . . . *
        // . . o .
        // . * . .
        // .     *
        Op resumeFrom[] = {
            Op::NOOP, Op::NOOP, Op::NOOP, Op::DATA, Op::TURN,
            Op::NOOP, Op::NOOP, Op::NOOP, Op::TURN,
            Op::NOOP, Op::NOOP, Op::TURN,
            Op::NOOP, Op::TURN,
            Op::DATA, Op::TURN,
            Op::UNSET
        };
        searcher->findOne(resumeFrom);

        REQUIRE(tracker->getTotalEarlyHangs() == 1);
    }
    SECTION( "InfSeq1" ) { // Extends leftwards
        //
        // *   *
        // o . . . *
        // . * o .
        // .   . *
        // .   *
        Op resumeFrom[] = {
            Op::NOOP, Op::NOOP, Op::NOOP, Op::DATA, Op::TURN,
            Op::NOOP, Op::NOOP, Op::NOOP, Op::TURN,
            Op::NOOP, Op::TURN,
            Op::DATA, Op::TURN,
            Op::NOOP, Op::TURN, Op::TURN,
            Op::TURN,
            Op::UNSET
        };
        searcher->findOne(resumeFrom);

        REQUIRE(tracker->getTotalEarlyHangs() == 1);
    }
    SECTION( "InfSeq2" ) { // Extends rightwards
        //       *
        // *   * o *
        // o . . o *
        // . * . o
        // .   . *
        Op resumeFrom[] = {
            Op::NOOP, Op::NOOP, Op::DATA, Op::TURN,
            Op::NOOP, Op::NOOP, Op::DATA, Op::TURN,
            Op::DATA, Op::TURN, Op::TURN,
            Op::DATA, Op::TURN,
            Op::NOOP, Op::TURN,
            Op::TURN,
            Op::UNSET
        };
        searcher->findOne(resumeFrom);

        REQUIRE(tracker->getTotalEarlyHangs() == 1);
    }

    delete searcher;
    delete tracker;
}
