//
//  HangTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 03/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>
#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "5x5 Hang tests", "[hang][5x5]" ) {
    ExhaustiveSearcher *searcher = new ExhaustiveSearcher(5, 5, 64);
    ProgressTracker *tracker = new ProgressTracker(searcher);

    searcher->setProgressTracker(tracker);
    searcher->setHangSamplePeriod(128);

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
    SECTION( "NonUniformCountingLoop1" ) {
        // Loop that increases counter, but with some instructions executed more frequently than
        // others. Furthermore, another data cell switches between three possible values, including
        // zero.
        //
        //     * *
        // *   o o *
        // o . . o *
        // . * o . *
        // .     *
        Op resumeFrom[] = {
            Op::NOOP, Op::NOOP, Op::DATA, Op::TURN,
            Op::NOOP, Op::NOOP, Op::DATA, Op::TURN,
            Op::DATA, Op::TURN, Op::TURN,
            Op::NOOP, Op::TURN, Op::TURN,
            Op::DATA, Op::TURN,
            Op::DATA, Op::TURN, Op::TURN, Op::TURN,
            Op::UNSET
        };
        searcher->findOne(resumeFrom);

        REQUIRE(tracker->getTotalEarlyHangs() == 1);
    }
    SECTION( "NonUniformCountingLoop2" ) {
        //     * *
        // * o o o *
        //   . o o *
        // * * o . *
        // o o o *
        Op resumeFrom[] = {
            Op::DATA, Op::TURN,
            Op::DATA, Op::DATA, Op::TURN,
            Op::DATA, Op::DATA, Op::DATA, Op::TURN,
            Op::DATA, Op::TURN, Op::TURN,
            Op::DATA, Op::TURN,
            Op::NOOP, Op::TURN,
            Op::DATA, Op::TURN,
            Op::NOOP, Op::TURN, Op::TURN,
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
        // .     *
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
    SECTION( "InfSeqNonUniform1" ) {
        // Loop that generates an infinite sequence where some instructions are executed more
        // frequently than others.
        //
        //   * * *
        // * . o o *
        // o . . . *
        // . * . .
        // .     *
        Op resumeFrom[] = {
            Op::NOOP, Op::NOOP, Op::DATA, Op::TURN,
            Op::NOOP, Op::NOOP, Op::NOOP, Op::TURN,
            Op::NOOP, Op::TURN,
            Op::NOOP, Op::TURN,
            Op::DATA, Op::TURN,
            Op::DATA, Op::TURN, Op::TURN,
            Op::NOOP, Op::TURN, Op::TURN,
            Op::UNSET
        };
        searcher->findOne(resumeFrom);

        REQUIRE(tracker->getTotalEarlyHangs() == 1);
    }
    SECTION( "InfSeqNonUniform2" ) {
        //   * * *
        // * . o o *
        // o . o . *
        // . * . . *
        // .     *
        Op resumeFrom[] = {
            Op::NOOP, Op::NOOP, Op::DATA, Op::TURN,
            Op::NOOP, Op::DATA, Op::NOOP, Op::TURN,
            Op::DATA, Op::TURN, Op::TURN,
            Op::NOOP, Op::TURN,
            Op::NOOP, Op::TURN,
            Op::DATA, Op::TURN,
            Op::NOOP, Op::TURN, Op::TURN,
            Op::UNSET
        };
        searcher->findOne(resumeFrom);

        REQUIRE(tracker->getTotalEarlyHangs() == 1);
    }

    delete searcher;
    delete tracker;
}
