//
//  CompletionTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 03/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>


#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "6x6 Completion tests", "[success][6x6]" ) {
    ExhaustiveSearcher searcher(6, 6, 32);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);
    searcher.setMaxStepsPerRun(10000);
    searcher.setMaxStepsTotal(10000);
    searcher.setHangSamplePeriod(64);

    SECTION( "DivergingDeltaYetNoHang" ) {
        // A diverging change after 102 steps, however, not a hang as one value change touched zero
        // (it changed from one, to zero, back to one). At Step 102 this value is two (a diverging
        // delta) but it does not touch zero anymore, which means that subsequent program flow
        // diverges.
        Op resumeFrom[] = {
            Op::NOOP, Op::DATA, Op::DATA, Op::DATA, Op::TURN, Op::NOOP, Op::NOOP, Op::TURN,
            Op::DATA, Op::DATA, Op::TURN, Op::TURN, Op::NOOP, Op::TURN, Op::DATA, Op::DATA,
            Op::TURN, Op::TURN, Op::NOOP, Op::NOOP, Op::DATA, Op::DATA, Op::TURN, Op::DATA,
            Op::NOOP, Op::TURN, Op::TURN, Op::NOOP, Op::NOOP, Op::TURN, Op::TURN, Op::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalSuccess() == 1);
    }
    SECTION( "IdenticalSnapshotDeltaButNonZeroNewlyVisited" ) {
        // An example where after 220 steps the snapshot delta is the same, but a newly visited
        // value was not zero. This hang was not detected, as there was an error in the check for
        // sequences extending to the left.
        Op resumeFrom[] = {
            Op::DATA, Op::TURN, Op::DATA, Op::TURN, Op::NOOP, Op::NOOP, Op::DATA, Op::TURN,
            Op::DATA, Op::DATA, Op::TURN, Op::DATA, Op::TURN, Op::NOOP, Op::TURN, Op::DATA,
            Op::TURN, Op::TURN, Op::DATA, Op::NOOP, Op::DATA, Op::NOOP, Op::TURN, Op::DATA,
            Op::TURN, Op::NOOP, Op::TURN, Op::TURN, Op::TURN, Op::TURN, Op::TURN, Op::TURN,
            Op::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalSuccess() == 1);
    }
}

TEST_CASE( "7x7 Completion tests", "[success][7x7]" ) {
    ExhaustiveSearcher searcher(7, 7, 1024);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);
    searcher.setMaxStepsPerRun(1000000);
    searcher.setMaxStepsTotal(1000000);
    searcher.setHangSamplePeriod(256);

    SECTION( "BB 7x7 #117272" ) {
        Op resumeFrom[] = {
            Op::NOOP, Op::NOOP, Op::DATA, Op::TURN, Op::NOOP, Op::NOOP, Op::DATA, Op::DATA,
            Op::TURN, Op::DATA, Op::TURN, Op::NOOP, Op::TURN, Op::NOOP, Op::TURN, Op::NOOP,
            Op::TURN, Op::NOOP, Op::TURN, Op::DATA, Op::DATA, Op::TURN, Op::DATA, Op::NOOP,
            Op::TURN, Op::TURN, Op::DATA, Op::TURN, Op::TURN, Op::DATA, Op::NOOP, Op::TURN,
            Op::DATA, Op::TURN, Op::TURN, Op::NOOP, Op::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 117272);
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
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 140163);
    }
    SECTION( "BB 7x7 #422154" ) {
        Op resumeFrom[] = {
            Op::DATA, Op::NOOP, Op::NOOP, Op::NOOP, Op::TURN, Op::NOOP, Op::TURN, Op::DATA,
            Op::TURN, Op::NOOP, Op::DATA, Op::DATA, Op::TURN, Op::DATA, Op::TURN, Op::NOOP,
            Op::TURN, Op::DATA, Op::DATA, Op::TURN, Op::NOOP, Op::NOOP, Op::TURN, Op::TURN,
            Op::DATA, Op::TURN, Op::NOOP, Op::DATA, Op::TURN, Op::NOOP, Op::NOOP, Op::TURN,
            Op::NOOP, Op::NOOP, Op::TURN, Op::DATA, Op::NOOP, Op::NOOP, Op::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 422154);
    }
}
