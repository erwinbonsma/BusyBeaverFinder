//
//  FailingHangTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 17/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>
#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "6x6 Failing Hang tests", "[hang][regular][sweep][6x6][fail]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxSteps = 1000000;
    searcher.configure(settings);

    SECTION( "6x6-SweepWithInSequenceOscillatingZeros" ) {
        // The body of the sweep consists of alternating -1 and 0 values. The left side is a fixed
        // but increasing value, the right side of the sequence steadily grows. The leftward sweep
        // inverts the entire sweep body. The -1 values become zeroes, and vice versa. Even though
        // zero is an exit of the rightward sweep, this sweep moves DP two positions and is always
        // aligned such that it uses the -1 values as stepping stones.
        //
        //       * *
        //   * * o _ *
        // * _ o o _ *
        // * _ _ _ _ *
        // * * o _ *
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
    SECTION( "6x6-SweepWithInSequenceOscillatingValues" ) {
        // Similar to the previous program, but here the sequence body consists of -1 and -2
        // values. The leftward sweep swaps these values every sweep.
        //
        //     * * *
        //   * _ o o *
        //   * o o *
        // * o _ _ _ *
        // *   * o o
        // o _ _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
    SECTION( "6x6-SweepWithIrregularFixedPointGrowingValue" ) {
        // The right end of the sequence looks as follows: [body] 1 X 1 0 0 0
        // The rightward sweep loop moves DP two units. Half of the sweeps it ends on the first
        // 1. In that case, the loop exits and returns without making any modifications. The other
        // half of the sweeps and on the second 1. In that case, it increases the positive value
        // X by one.
        //
        //       *
        //   * * o _ *
        // * _ o o *
        // * o o _ _
        // * * _ o _ *
        // o _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
    SECTION( "6x6-LateStartingPeriodicSweepWithTwoFastGrowingEnds" ) {
        // This program executes a complex sweep that looks to be irregular but seems to becomes
        // regular eventually. TODO: Check why the meta-run summary does not reflect this.
        //
        //     * * *
        //   * o o _ *
        //   * * o _
        //   _ o o *
        // * _ _ o _ *
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
}

TEST_CASE( "6x6 Failing Irregular Sweep Hang tests", "[hang][sweep][irregular][6x6][fail]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectionSteps = 20000;
    settings.maxSteps = 1000000;
    searcher.configure(settings);

    SECTION( "6x6-IrregularSweepWithZeroesInAppendix" ) {
        // A truly binary counter. It actually uses ones and zeros, and also properly generates
        // binary numbers (only with most-significant bit at the right).
        //
        //   *   * *
        // * o _ _ _ *
        // o o * o o *
        // o   * o *
        // _ * _ o *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
}

TEST_CASE( "6x6 Failing Irregular Other Hangs", "[hang][irregular][6x6][fail]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectionSteps = 20000;
    settings.maxSteps = 1000000;
    searcher.configure(settings);

    SECTION( "6x6-IrregularHopScotch" ) {
        // This program executes a curious sweep. The sweep does not have a sweep body. At the left
        // there's a fixed, ever-increasing exit. Directly attached is an a-periodically growing
        // appendix which consists of three values: 0, -1 and -2.
        //
        // The right-moving loop moves DP two steps. It increases the values it lands on by one,
        // converting -2's to -1's. It exits on zero.
        //
        // The left-moving sweep is more chaotic. It clears -1 values and leaves -2 values behind.
        // It only converts half of the -1 values to -2. The others it resets to zero.
        //
        //       * *
        //   * * o _ *
        // * o o o _ *
        // * _ _ _ _ *
        // * * o o *
        // o _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
}

TEST_CASE( "7x7 hangs", "[hang][7x7][fail]" ) {
    ExhaustiveSearcher searcher(7, 7, 16384);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectionSteps = 100000;
    settings.maxSteps = settings.maxHangDetectionSteps;
    settings.undoCapacity = settings.maxSteps;
    searcher.configure(settings);

    SECTION( "7x7-UndetectedHang1" ) {
        // *   *   * *
        // o _ _ _ _ _ *
        // _   _ * _ _
        // _ * o o o o *
        // _   * o o o *
        // _ * _ o o *
        // _   * * *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
    SECTION( "7x7-UndetectedHang2" ) {
        // Irregular sweep.
        //
        // TODO: Analyze why hang is not detected with current logic.
        //
        // *       *
        // o _ * _ o _ *
        // _ _ * * o _
        // _ _ o o _ *
        // _ o _ * o _
        // _ * * _ o o *
        // _       * *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
    SECTION( "7x7-UndetectedHang3" ) {
        // Program that causes Periodic Sweep Hang Detector to create transition-groups with
        // unique transitions. This caused an assertion to fail, which after analysis, has been
        // removed.
        //
        // *     * *
        // o _ _ _ _ _ *
        // _   * _ o _
        // _ * o o o *
        // _   _ o _
        // _ * _ _ _ o *
        // _   * * * *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
}
