//
//  CompletionTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 03/02/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#include <stdio.h>


#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "Completion tests", "[success]" ) {
    ExhaustiveSearcher *searcher = new ExhaustiveSearcher(7, 7, 1024);
    ProgressTracker *tracker = new ProgressTracker(searcher);

    searcher->setProgressTracker(tracker);
    searcher->setMaxStepsPerRun(1000000);
    searcher->setMaxStepsTotal(1000000);
    searcher->setHangSamplePeriod(256);

    SECTION( "BB 7x7 #117272" ) {
        Op resumeFrom[] = {
            Op::NOOP, Op::NOOP, Op::DATA, Op::TURN, Op::NOOP, Op::NOOP, Op::DATA, Op::DATA,
            Op::TURN, Op::DATA, Op::TURN, Op::NOOP, Op::TURN, Op::NOOP, Op::TURN, Op::NOOP,
            Op::TURN, Op::NOOP, Op::TURN, Op::DATA, Op::DATA, Op::TURN, Op::DATA, Op::NOOP,
            Op::TURN, Op::TURN, Op::DATA, Op::TURN, Op::TURN, Op::DATA, Op::NOOP, Op::TURN,
            Op::DATA, Op::TURN, Op::TURN, Op::NOOP, Op::UNSET
        };
        searcher->findOne(resumeFrom);

        REQUIRE(tracker->getMaxStepsFound() == 117272);
    }
    SECTION( "BB 7x7 #140164" ) {
        //   *   * * *
        // * o . . o . *
        // . o * o o .
        // . * . o o *
        // .   . * o .
        // . * . . . . *
        // .       *
        Op resumeFrom[] = {
            Op::DATA, Op::NOOP, Op::NOOP, Op::NOOP, Op::NOOP, Op::TURN, Op::DATA, Op::TURN,
            Op::DATA, Op::TURN, Op::NOOP, Op::NOOP, Op::DATA, Op::NOOP, Op::TURN, Op::TURN,
            Op::NOOP, Op::TURN, Op::DATA, Op::DATA, Op::DATA, Op::TURN, Op::NOOP, Op::TURN,
            Op::DATA, Op::DATA, Op::NOOP, Op::TURN, Op::NOOP, Op::TURN, Op::NOOP, Op::TURN,
            Op::NOOP, Op::NOOP, Op::TURN, Op::NOOP, Op::TURN, Op::UNSET,
        };
        searcher->findOne(resumeFrom);

        REQUIRE(tracker->getMaxStepsFound() == 140163);
    }
    SECTION( "BB 7x7 #422154" ) {
        Op resumeFrom[] = {
            Op::DATA, Op::NOOP, Op::NOOP, Op::NOOP, Op::TURN, Op::NOOP, Op::TURN, Op::DATA,
            Op::TURN, Op::NOOP, Op::DATA, Op::DATA, Op::TURN, Op::DATA, Op::TURN, Op::NOOP,
            Op::TURN, Op::DATA, Op::DATA, Op::TURN, Op::NOOP, Op::NOOP, Op::TURN, Op::TURN,
            Op::DATA, Op::TURN, Op::NOOP, Op::DATA, Op::TURN, Op::NOOP, Op::NOOP, Op::TURN,
            Op::NOOP, Op::NOOP, Op::TURN, Op::DATA, Op::NOOP, Op::NOOP, Op::UNSET
        };
        searcher->findOne(resumeFrom);

        REQUIRE(tracker->getMaxStepsFound() == 422154);
    }

    delete searcher;
    delete tracker;
}
