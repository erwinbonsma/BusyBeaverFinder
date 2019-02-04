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

TEST_CASE( "Hang tests", "[hang]" ) {
    ExhaustiveSearcher *searcher = new ExhaustiveSearcher(9, 9, 1024);
    ProgressTracker *tracker = new ProgressTracker(searcher);

    searcher->setProgressTracker(tracker);

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
    SECTION( "InfSeq1" ) {
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
    SECTION( "InfSec2" ) {
        //       *
        // *   * o *
        // o . . o *
        // . . o .
        // . * . .
        // .     *
        Op resumeFrom[] = {
            Op::NOOP, Op::NOOP, Op::NOOP, Op::DATA, Op::TURN,
            Op::NOOP, Op::NOOP, Op::DATA, Op::TURN,
            Op::DATA, Op::TURN, Op::TURN,
            Op::NOOP, Op::NOOP, Op::TURN,
            Op::NOOP, Op::TURN,
            Op::DATA, Op::TURN,
            Op::UNSET
        };
        searcher->findOne(resumeFrom);

        REQUIRE(tracker->getTotalEarlyHangs() == 1);
    }

    delete searcher;
    delete tracker;
}
