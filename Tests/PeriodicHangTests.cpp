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

        REQUIRE(tracker.getTotalHangs(HangType::NO_DATA_LOOP) == 1);
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
    settings.maxHangDetectionSteps = 2048;
    settings.maxSteps = settings.maxHangDetectionSteps;
    // Prevent No Exit hang detection from also catching some of the hangs below as is test case
    // is testing the Periodic Hang Detector
    settings.disableNoExitHangDetection = true;
    searcher.configure(settings);

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
    SECTION( "6x6-PeriodicHangWithInnerLoop" ) {
        // Periodic hang which generates a sequence that extends to the right. It was not detected
        // by an earlier version of the Run Summary-based Periodic Hang Detector as the hang loop
        // contains itself a loop.
        //
        // The Run Summary is as follows:
        // #14(74 60 79 80 74 61 40 79 81)  - switch
        // #5(75 80 75 80 75 80)            - inner-loop, always executed three times
        // ... etc
        //
        //   *   * *
        // * _ o o o *
        // o _ o _ *
        // o * o _ *
        // o   * *
        // o
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::NOOP,
            Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "6x6-PeriodicHangWithInnerLoop2" ) {
        // Another periodic hang with an inner loop.
        //
        //   * * *
        // * _ o o *
        // o _ o * _
        // o _ _ _ _ *
        // o * _ o o
        // _       *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "6x6-PeriodicHangWithInnerLoop3" ) {
        // Another periodic hang with an inner loop.
        //
        //   *   * *
        // * _ o o o *
        // o _ _ o o *
        // o * * _ o
        // o       *
        // _
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
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
    SECTION( "6x6-NonUniformCountingLoop2" ) {
        // Periodic hang which changes two values. One value is decreased by one each iteration,
        // another is increased by three each iteration. A third value oscillates between -3 and 0.
        //
        // Note: This program only differs in one instruction from the previous one, but its
        // execution differs significantly.
        //
        //       * *
        //   * * _ o *
        //   o _ o o *
        //   _ * o o *
        // * _ o o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN,
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
        // It was not detected by an earlier version of the Run Summary-based Periodic Hang Detector
        // as the hang loop contains itself a loop.
        //
        // #14(78 81 60)
        // #4(25 43 25 43 25 43 25 43 25)
        // #11(42 79 60 25 42)
        // #8(79 61 79 61 79 61 79 61 79 61 79 61 79 61 79 61)
        // #14(78 81 60)
        // ... etc
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
    SECTION( "6x6-DelayedHang3" ) {
        // Variant of the previous hang. However, it is sufficiently different that it revealed a
        // bug in the Sweep Hang detector, which was not detected by the previous test.
        //
        //     *
        // * _ o _ _ *
        //   * o o _
        //   o o * *
        // * _ o _ _ *
        // o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "6x6-DelayedHang4" ) {
        // Another variant of the previous hang. It is added as this program set the record in the
        // 6x6 search for requiring the most hang detection attempts before being successfully
        // detected.
        //
        //       *
        //     * o _ *
        //   * * o _
        //   _ o o *
        // * _ _ o
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "6x6-PeriodicHangWithThreeInnerLoops" ) {
        // Periodic hang with a period of 136 steps. Each iteration it extends the sequence to the
        // left with four values "1 4 4 1".
        //
        // Each iteration consists of the following run blocks:
        // #22[77 81 77 81 77 81]
        // #34(76 23 57 61 25 57 61 24 39 57 60 79)
        // #22[77 81 77 81 77 81]
        // #51[76 23 57 61 24 39 57 60 79 77 81
        //     76 23 57 61 24 39 57 60 79 77 81
        //     76 23 57 61]
        // #59(25 57 61 24 39 57 60 79)
        //
        //     * * *
        //   * o o _ *
        //   o o o *
        // * o _ o _
        // * _ _ _ _ *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "6x6-PeriodicHangBreakingOutOfAssumedMetaLevelLoop" ) {
        // A periodic hang at the low-level run summary which initially looks like a hang at the
        // meta-run level, but this assumed endless meta-loop is exited. This caused an earlier
        // periodic hang detection implementation to hang.
        //
        //   *     *
        // * o _ _ _ *
        //   o *   o
        // * _ _ o o *
        // * * _ * o
        // o o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "6x6-PeriodicHangBreakingOutOfAssumedMetaLevelLoop2" ) {
        // Another program that enters a loop at meta-level but switches enters an endless
        // (periodic) loop at the lower-level without breaking out of the meta-level loop. It
        // caused earlier versions of the Glider and Meta Periodic hang detectors to hang.
        //
        // Run summary:
        // #2(39 75)
        // #9[38 44 81 45] - twice
        // #2(39 75)
        // #9[38 44 81 45] - endless
        //
        //   *     *
        //   _ *   o *
        //   o o o o *
        // * o o _ _ *
        // * * _   *
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
    SECTION( "6x6-PeriodicHangBreakingOutOfAssumedMetaLevelLoop3" ) {
        // Initially this program appears to carry out an irregular sweep. However, after about 280
        // steps, it gets locked into a simple periodic loop.
        //
        //   *   *
        // * o o o _ *
        // o _ o o *
        // o * * _
        // o * _ o *
        // o     *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::PERIODIC) == 1);
    }
}

TEST_CASE( "7x7 Periodic Hang tests", "[hang][periodic][7x7]" ) {
    ExhaustiveSearcher searcher(7, 7, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxSteps = 1000000;
    searcher.configure(settings);

    SECTION( "7x7-DelayedPeriodicHang") {
        // Program that enters a periodic hang after around 800 steps. Earlier versions of the
        // hang detection failed to detect it, as it filled the run history buffer, forcing a
        // premature abort of the hang detection.
        //
        //         *
        //     * * o _ *
        //     _ _ o *
        //   * o o o _ *
        //   o _ o *
        // * _ _ o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalDetectedHangs() == 1);
    }
}
