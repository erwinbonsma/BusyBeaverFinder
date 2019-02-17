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

TEST_CASE( "5x5 Periodic Hang tests", "[hang][periodic][5x5]" ) {
    ExhaustiveSearcher searcher(5, 5, 64);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    searcher.configure(settings);

    SECTION( "BasicLoop" ) {
        // *   *
        // o . . . *
        // . * . .
        // .     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
    SECTION( "CountingLoop" ) {
        // *   *
        // o . . . *
        // . . o .
        // . * . .
        // .     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
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
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
    SECTION( "NonUniformCountingLoop2" ) {
        //     * *
        // * o o o *
        //   . o o *
        // * * o . *
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
    SECTION( "InfSeq1" ) { // Extends leftwards
        //
        // *   *
        // o . . . *
        // . * o .
        // .   . *
        // .   *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
    SECTION( "InfSeq2" ) { // Extends rightwards
        //       *
        // *   * o *
        // o . . o *
        // . * . o
        // .     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
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
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
    SECTION( "InfSeqNonUniform2" ) {
        //   * * *
        // * . o o *
        // o . o . *
        // . * . . *
        // .     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
    SECTION( "InfSeqNonUniform3" ) {
        // The period of this hang loop is a multiple of the period of the evaluated instructions.
        // When the hang cycle is executed once, the instructions have been repeated twice. So when
        // the cycle detector would only consider the latter it would not detect the hang. The hang
        // is only detected when also the PP-direction is taken into account.
        //
        //   * * *
        // * . o o *
        // o . o o *
        // . * . . *
        // .     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
}

TEST_CASE( "5x5 Sweep Hang tests", "[hang][sweep][5x5]" ) {
    ExhaustiveSearcher searcher(5, 5, 64);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    searcher.configure(settings);

    SECTION( "InfSeqExtendingBothWays1" ) {
        //     *
        //   * o . *
        // * . o *
        // * o *
        // o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
    SECTION( "InfSeqExtendingBothWays2" ) {
        //     *
        //   * o . *
        //   o o *
        // * . o *
        // o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
}

TEST_CASE( "6x6 Periodic Hang tests", "[hang][periodic][6x6]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectAttempts = 4;
    settings.initialHangSamplePeriod = 16;
    searcher.configure(settings);

    SECTION( "6x6-NonUniformCountingLoop1" ) {
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
}

TEST_CASE( "6x6 Sweep Hang tests", "[hang][sweep][6x6]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectAttempts = 4;
    settings.initialHangSamplePeriod = 16;
    searcher.configure(settings);

    SECTION( "6x6-InfSweepSeqExtendingOneWay") {
        // This program sweeps over the entire data sequence, which causes the hang cycle to
        // continuously increase. However, it only extends one way.

        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
    SECTION( "6x6-InfSweepSeqExtendingOneWayWithZeroes") {
        // Here a sweep is occuring over a zero-delimited part of the sequence.
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
}

TEST_CASE( "6x6 Failing Hang tests", "[hang][sweep][6x6][.fail]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectAttempts = 4;
    settings.initialHangSamplePeriod = 16;
    searcher.configure(settings);

    SECTION( "6x6-HangGlider") {
        // Here a sweep is occuring over part of a sequence, where the midway point is a temporary
        // zero. It has value one, which only briefly becomes zero, triggering the turn after
        // which its value is restored to one.
        //
        // The name of this section is based on the program's shape. The type of hang is more
        // accurately described by 6x6-InfSweepSeqExtendingOneWayWithNonZeroMidPoint
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
    SECTION( "6x6-RunAwayGlider") {
        // A non-periodic hang where two ever-increasing values move rightward on the tape, leaving
        // zeroes in their wake. The amount of steps required to move one position on the tape
        // doubles each time (as the left counter is decreased towards zero, the right counter is
        // increased by two).
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalEarlyHangs() == 1);
    }
}
