//
//  PeriodicHangTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 17/03/19.
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
    // Prevent No Exit hang detection from also catching some of the hangs below as is test case
    // is testing the Periodic Hang Detector
    settings.disableNoExitHangDetection = true;
    searcher.configure(settings);

    SECTION( "BasicLoop" ) {
        // Classification: Periodic, Constant, Uniform, Stationary
        //
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

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "CountingLoop" ) {
        // Classification: Periodic, Changing, Uniform, Stationary
        //
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

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "NonUniformCountingLoop1" ) {
        // Classification: Periodic, Changing, Non-uniform, Sentry Go
        //
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

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "NonUniformCountingLoop2" ) {
        // Classification: Periodic, Changing, Non-uniform, Sentry Go
        //
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

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "InfSeq1" ) {
        // Classification: Periodic, Changing, Uniform, Travelling
        //
        // Sequence that extends leftwards
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

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "InfSeq2" ) {
        // Classification: Periodic, Changing, Uniform, Travelling
        //
        // Sequence that extends rightwards
        //
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

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "InfSeqNonUniform1" ) {
        // Classification: Periodic, Changing, Non-Uniform, Travelling
        //
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

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "InfSeqNonUniform2" ) {
        // Classification: Periodic, Changing, Non-Uniform, Travelling
        //
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

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "InfSeqNonUniform3" ) {
        // Classification: Periodic, Changing, Non-Uniform, Travelling
        //
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

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
}

TEST_CASE( "6x6 Periodic Hang tests", "[hang][periodic][6x6]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxPeriodicHangDetectAttempts = 6;
    settings.initialHangSamplePeriod = 16;
    settings.maxSteps = 2048;
    // Prevent No Exit hang detection from also catching some of the hangs below as is test case
    // is testing the Periodic Hang Detector
    settings.disableNoExitHangDetection = true;
    searcher.configure(settings);

    SECTION( "6x6-NonUniformCountingLoop1" ) {
        // Classification: Periodic, Changing, Non-Uniform, Sentry Go
        //
        // Two values oscillate between zero and non-zero values. A third value is changing by one
        // each cycle.
        //
        //       * *
        //   * * _ o *
        //   o _ o o *
        //   _ * o _ *
        // * _ o o *
        // o o * *
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

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "6x6-DelayedHang") {
        // Classification: Periodic, Constant, Non-Uniform(?), Travelling
        //
        //   * *   *
        // * o o o _ *
        //   * o o o *
        // * _ o * *
        // * o *
        // o o *
        //
        // For this program it takes a relatively long time before the hang is started. The hang
        // only starts at Step 158.
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::TURN,Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "6x6-DelayedHang2" ) {
        // A complex periodic hang. The hang period is 82 steps, the periodic execution only starts
        // around step 410, and every period it extends the sequence with three cells.
        // It generates the following sequence: -2 4 2 -2 4 2 -2 4 2 -2 4 2 -1 3 2 -2 3 1 -1 3 2 0
        // The periodic loop goes over the last nine values, leaving -2 4 2 in its wake.
        //
        //       *
        //   * _ o _ *
        //   * * o _
        //   _ o o *
        // * _ _ o
        // o _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
}

