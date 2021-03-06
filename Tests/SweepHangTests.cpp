//
//  SweepHangTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 03/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>
#include "catch.hpp"

#include "ExhaustiveSearcher.h"
#include "SweepHangDetector.h"

TEST_CASE( "5x5 Sweep Hang tests", "[hang][sweep][regular][5x5]" ) {
    ExhaustiveSearcher searcher(5, 5, 64);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxSteps = 2048;
    // Prevent No Exit hang detection from also catching some of the hangs below as this test case
    // is testing the Regular Sweep Hang Detector
    settings.disableNoExitHangDetection = true;
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

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);
    }
    SECTION( "InfSeqExtendingBothWays2" ) {
        // Sequence that extends both ways, but to the right it extends only at half the speed.
        //
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

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);
    }
}

TEST_CASE( "6x6 Sweep Hang tests", "[hang][sweep][regular][6x6]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectionSteps = 16384;
    settings.maxSteps = settings.maxHangDetectionSteps;
    // Prevent No Exit hang detection from also catching some of the hangs below as is test case
    // is testing the Regular Sweep Hang Detector
    settings.disableNoExitHangDetection = true;
    searcher.configure(settings);

    SECTION( "6x6-SweepExtendingLeftwards") {
        // This program sweeps over the entire data sequence, which causes the hang cycle to
        // continuously increase. However, it only extends one way.
        //
        //     * *
        //   * o _ _ *
        //   o _ o *
        //   _ _ _ o *
        // * _ _ _ _ *
        // o o * * *
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

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepExtendingRightwards" ) {
        // The transition at the left features a double shift, a feature that caused problems for
        // early hang detectors. The program generates a sequence that increases towards zero:
        // .... -5 -4 -3 -2 -1 0
        //
        //         *
        //   * _ _ _
        //   _ _ * _
        // * o o _ o *
        // * * * _ _
        // o _ _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepExtendingRightwardsWithNoisyLeftSweep" ) {
        // Regular sweep with a "noisy" left sweep. DP moves two cells left, then one cell right,
        // etc.
        //
        //     * *
        //   * o o _ *
        // * _ _ o *
        // * o * *
        // o o *
        // o
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN,Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepExtendingLeftwardsWithNoisyRightSweep" ) {
        // Similar to previous, but now the double-shift occurs when moving rightwards. Another
        // noteworthy feature of this program is that it generates an ever-growing sequence of -2
        // values, followed by a positive value that equals the number of -2 values.
        //
        // *     *
        // o _ * o _ *
        // _ o _ _ *
        // _ * * o _
        // _ * _ o o *
        // _     * *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepExtendingLeftwardsWithZeroFixedMidSequenceTurn") {
        // Here a sweep is occuring over a zero-delimited part of the sequence. The right-going
        // sweep ends at a mid-sequence zero.
        //
        //     * *
        //   * o _ _ *
        //   o o _ *
        // * _ _ o *
        // * _ o *
        // o o *
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

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepExtendingLeftwardsWithNonZeroFixedMidSequenceTurn") {
        // Here a sweep is occuring over part of a sequence, where the midway point is a temporary
        // zero. It has value one, which only briefly becomes zero, triggering the turn after
        // which its value is restored to one.
        //
        //     * *
        //   * o _ _ *
        //   o o _ *
        //   _ o o
        // * _ _ _ *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepWithNonZeroFixedPointThatOscillatesDuringTurn" ) {
        // Here the right-going sweep ends at a value 1. During the transition the value is
        // temporarily changed to zero, then restored to 1 again, before starting the left-going
        // sweep.
        //
        // *     *
        // o _ * o _ *
        // _ o _ o *
        // _ * _ o _ *
        // _     *
        // _
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepWithNonZeroFixedPointThatOscillatesDuringTurn2") {
        // A single-headed sweep. The fixed point has value 1, which is changed to zero when the
        // loop exit. The subsequent transition restores the value to 1 again.
        //
        // *     *
        // o _ * o _ *
        // _ _ o o *
        // _ o _ _ *
        // _ * _ o _ *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepWithNonZeroFixedPointThatOscillatesDuringTurn3" ) {
        // Hang featuring a complex fixed turn at the right side of the sequence. The sweep loop
        // ends on value 1 (with it becoming zero). The transition changes this value to 2, and the
        // left-sweeping loop finally restores it to 1.
        //
        // *     *
        // o _ * o _ *
        // _ _ * o o
        // _ o _ o *
        // _ * _ o _ *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepWithNonZeroFixedPointThatOscillatesDuringTurn4" ) {
        // The right-sweep exits on a fixed point, with value one. When the loop exits, it is zero.
        // The transition increases it to two, with the left-sweeping loop restoring it to one
        // again.
        //
        //   *   *
        // * o * o _ *
        // o o * o o
        // _ _ _ o *
        // _ * _ o _ *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepWithZeroFixedPointThatOscillatesDuringTurn" ) {
        // A leftwards extending sweep that turns at the right on a zero value. During the turn,
        // this value briefly oscillates to 1 before it is restored to zero. The logic of this
        // reveral is fairly complex. A noteworthy feature of this program is that it generates a
        // sequence of descending values: -1 -2 -3 -4 -5 .... etc
        //
        //   *   * *
        // * o o _ _ *
        // o _ o _ _ *
        // o * * o o *
        // o * _ o *
        // o     *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepWithMidSweepNonZeroFixedPointThatOscillatesDuringTurn" ) {
        // Similar to the previous program, but now the fixed turn at the right is mid-sequence.
        //
        //       *
        // * _ * o _ *
        // o o _ o *
        // _ * _ o _ *
        // _     *
        // _
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepWithMidSweepNonZeroFixedPointThatOscillatesDuringTurn2" ) {
        // Similar in behaviour to the previous program. Furthermore, when sweeping leftwards, it
        // shifts left twice, which prevented it from being detected by an early hang detection
        // algorithm.
        //
        //     * *
        //   * o o _ *
        //   o o _ *
        // * _ _ o *
        // * _ o *
        // o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepWithMidSweepNonZeroFixedPointThatOscillatesDuringTurn3" ) {
        // Similar in behavior to the previous program, but this time the mid-sweep turn is at the
        // left of the sequence. The value that causes the exit is -1, on exit it is zero, it is
        // decreased to -2 by the transition, and restored to -1 by the right-sweeping loop.
        //
        //   *   *
        // * _ o o _ *
        //   _ * o _
        // * o o o *
        // * * _ o
        // o _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithMidSweepNonZeroFixedPointAndOutsideCounter" ) {
        // Sweep with a mid-sequence fixed point at its left side, which at its left (outside the
        // sweep sequence) has a counter that increases each sweep.
        //
        //     * * *
        //   * _ o o *
        //   o _ o *
        // * _ _ o _
        // * o o o _ *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-DualHeadedSweepHang") {
        // Dual-headed sweep hang which extends sweep with 3's at its left, and 2's at its right.
        // The latter value is realized in two sweeps. The end-point at the right has value 1.
        //
        //       *
        //     * o _ *
        //     o o *
        //   * _ o _
        // * _ _ o o *
        // o _ o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-DualHeadedSweepWithOscillatingExtension" ) {
        // Here the sweep reversal at the right side will first increment the zero turning value,
        // turn right twice as a result, then decrease it again (so it's back to zero), before
        // decreasing it once more to start the leftwards sweep.
        //
        //         *
        // *     * _
        // o o o _ _ *
        // o   * o o *
        // o * _ o *
        // o     *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-DualHeadedSweepWithFastRightSweep" ) {
        // Regular sweep with double-shift when moving rightwards.
        //
        //       *
        //   * * o _ *
        //   _ o o *
        // * _ _ _
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SingleHeadedSweepWithSlowLeftSweep") {
        // The left-sweep is simple but slow due to its relatively large program path.
        //
        //   * * *
        // * _ _ o *
        // o _ o * _
        // _ _ _ _ _ *
        // _ * o _ o
        // _       *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_INCREASING_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-DualHeadedSweepWithTwoNoisyShifts" ) {
        // This program features a negated double shift in both directions.
        //
        //     * * *
        //   * o o _ *
        // * _ o o *
        //   o * * *
        // * o _
        // o o _ *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-DualHeadedSweepWithTwoNoisyShifts2" ) {
        // Very similar to previous program. It constructs a palindrome that extends at both sides.
        // The sequence consists of negative values with increasingly larger (absolute) values
        // towards its center.
        //
        //     * * *
        //   * o o _ *
        // * _ o o *
        //   _ * _ _
        // * o _ o _ *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangConstructingDualHeadedPalindrome" ) {
        // Similar in behaviour to the previous program, but this one actually creates a perfect
        // palindrome. The program and path traversed by PP is also pretty.
        //
        //       *
        // * o _ o _ *
        // * _ * o _
        // o _ _ o *
        // _ * _ _
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-RegularSweepWithComplexReversal" ) {
        // A sweep with a mid-sequence reversal that takes 21 steps to execute. It features the
        // following operations: 3x SHL, 3x SHR, 3x INC, 2x DEC. The right-going sweep ends on the
        // value 1, which is reset to zero, DP then swifts two positions, gets back, restores the
        // value to 1, after which the leftwards sweep starts.
        //
        //       *
        //     * o _ *
        //   * _ o *
        // * o o _ _ *
        // * * _ o _ *
        // o o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepWithIrregularGrowth" ) {
        // The sweep extends rightwards, but the sequence is only extended once every two sweeps.
        // At the left, the sequence is bounded by a mid-sequence zero that oscillates during the
        // turn. DP also briefly exceeds this mid-sequence point during the transition
        //
        //       *
        //   * * o _ *
        // * o o o *
        // o o o _ *
        // o * _ o *
        // o _ _ *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::IRREGULAR_GROWTH);
    }
    SECTION( "6x6-SweepWithIrregularGrowth2" ) {
        //       *
        //     * o _ *
        //   * * o _
        //   _ o _ *
        // * _ _ o _ *
        // o _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalDetectedHangs() == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::IRREGULAR_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepIrregularGrowth3") {
        // Sweep with irregular growth at its left side. It alternates between two reversal
        // sequences:
        //
        // "... 0 0 1 1 [body]" => "... 0 1 0 3 [body]"
        // "... 0 0 1 0 [body]" => "... 0 0 1 1 [body]"
        //
        // A noteworthy feature is that the leftwards-loop exits at the same instruction in both
        // cases. The transition differs because a difference in a nearby data value.
        //
        //     * *
        //   * o _ _ *
        //   o _ o *
        //   o o o
        // * _ _ _ o *
        // o o * * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalDetectedHangs() == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::IRREGULAR_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepIrregularGrowth4" ) {
        // A sweep with irregular growth at its right side, and a fixed point with multiple values
        // at its left. It generates a sequence of only 2's, but does so quite slowly given that it
        // is a sweep that only adds a value once every two full sweeps.
        //
        //   *   * *
        // * o _ o _ *
        // * _ * o o
        // o _ o o *
        // _ * _ o *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_MULTIPLE_VALUES);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::IRREGULAR_GROWTH);
    }
    SECTION( "6x6-SweepIrregularGrowth5" ) {
        // Similar behavior as 6x6-SweepIrregularGrowth3.
        //
        //       *
        //   * * o _ *
        //   o o _ *
        // * o _ o _
        // * _ _ _ o *
        // o _ o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::IRREGULAR_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepIrregularGrowth6" ) {
        //   *   * *
        // * o o _ _ *
        //   _ o o o *
        //   * * o o *
        // * _ o o *
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,  Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::IRREGULAR_GROWTH);
    }
    SECTION( "6x6-SweepIrregularGrowth7" ) {
        // This dual-headed sweep has irregular growth at its left side. Two transitions alternate
        // there. Both start the same (from the same loop exit instruction), but deviate later due
        // to data difference at the left of the value that caused the sweep to abort.
        //
        //     * *
        //   * o o _ *
        //   o o _ *
        // * o _ o _
        // * _ _ _ o *
        // o o * * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::IRREGULAR_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepWithIrregularGrowth8" ) {
        // Sweep with irregular growth on its right side. It grows as follows:
        // [body] 2 3 2 0 0...
        // [body] 2 3 1 2 0...
        //   [body] 2 3 2 0...
        //
        // Furthermore, the program enters the hang meta-loop quite late. First it ends up executing
        // another sweep meta-loop, which however is not yet a hang (as this loop exits after three
        // iterations.
        //
        //     * * *
        //   * o o _ *
        //   * o o *
        //   o o o *
        // * _ o o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // Not actually correct. Although it executes irregular sweep behavior, this eventually
        // transfers into a normal sweep. For hang detection, this does not matter.
        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
    SECTION( "6x6-SweepLoopExceedsMidSequencePoint" ) {
        // Program where DP during its leftwards sweep briefly extends beyond the mid-sequence
        // point before initiating the turn.
        //
        //       * *
        //   * * _ o *
        //   _ _ o *
        // * o o o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-DualHeadedSweepExceedingRightEndSweepPoint" ) {
        // The sweep reversal at the right consists of two left-turns, at different data cells.
        //
        //     *   *
        //   * o o _ *
        // * _ o * *
        //   o *
        // * _
        // o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-DualHeadedSweepExceedingLeftEndSweepPoint" ) {
        // Similar to the previous program, but here DP briefly exceeds the sweep endpoint at the
        // left side of the sequence.
        ///
        //       *
        //   * * o _ *
        // * _ o o *
        // * * _ *
        // o _ o *
        // _
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-DualHeadedSweepExceedingLeftEndSweepPoint2" ) {
        // Similar to the previous hang, but it exceeds the sweep end-point by two shifts. The
        // sweep loop ends on -1, on exit it is 0, the transition does not change it, but the
        // right-sweeping loop restores it to -1.
        //
        //     *
        // * o o o _ *
        // * _ * o _
        // o _ o o *
        // o * _ o
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-ComplexSweepTurn1" ) {
        // Program with a complex turn at its left side. All values in the sequence are positive,
        // except the leftmost value, which is -1. When this is visisted, the following instructions
        // are executed, which together comprise the turn:
        //
        //         0   0 [-1] ...
        // INC 1   0   0 [ 0] ...
        // SHL 2 [ 0]  0   0  ...
        // SHR 2   0   0 [ 0] ...
        // INC 2   0   0 [ 2] ...
        // SHL 1   0 [ 0]  2  ...
        // DEC 2   0 [-2]  2  ...
        // INC 1   0 [-1]  2  ...
        // SHR 1   0  -1 [ 2] ...
        //
        //       *
        // * o o o _ *
        // * _ * o _
        // o _ o o *
        // o * _ o
        // o     *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-ComplexSweepTurn2" ) {
        // Hang similar to 6x6-ComplexSweepTurn1, but slightly simpler.
        //
        //       *
        // * o _ o _ *
        // * _ * o o
        // o _ _ o *
        // _ * _ _
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-ComplexSweepTurn3" ) {
        // Sweep hang with a complex fixed end point. The two left-most values of the sequence are
        // 1 and 2, and remain fixed. However, the transition briefly oscillates the neighbouring
        // zero to -2 and back to zero. Furthermore, it increases the value to the right of the
        // 2 by two, whereas the sweep increases the remainder of the sequence only by one.
        //
        //       *
        //     * o _ *
        // * _ o o *
        //   _ o o o *
        // * * _ _ _ *
        // o _ o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-ComplexSweepTurn4" ) {
        // The sweep turn on the right is complex. The turn value changes as follows:
        // 0 => 1 => -1 => 0 => -1 => -2
        //
        //   *     *
        //   _ _ * o *
        // * _ _ o o *
        // * o o _ o *
        // * * * _ _
        // o _ _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-ComplexSweepTurn5" ) {
        // Hang that was newly detected as a side-effect of sweep end-type classification
        // refactoring. It is not clear why detection initially failed.
        //
        //    * * *
        //   * o o _ *
        //   o o o *
        //   _ o o o *
        // * _ o _ _ *
        // o o * * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithVaryingLoopStarts" ) {
        // The hang has two possible transitions when reversing the sweep at the left side. It
        // depends on the last value, which is either -1 or 0. Depending on the transition, the
        // right-sweeping loop starts at a different instruction. In order to detect that the loop
        // is still the same in both cases, the hang detection needs to detect equivalence under
        // rotation.
        //
        //       *
        // * o _ o _ *
        //   _ * o _
        //   _ o o *
        // * * _ o
        // o _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_MULTIPLE_VALUES);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithVaryingLoopStarts2" ) {
        // This is a sweep with two different reversal sequences at the left side, which alternate.
        // - One reverses when it encounters 0, and leaves -1 behind
        // - The other reverses when it encounters -1, and leaves 0 behind
        //
        //       *
        // * _ _ o _ *
        // * _ * o _
        // o _ o o *
        // o * _ o
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_MULTIPLE_VALUES);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithVaryingLoopStarts3" ) {
        // Sweep with two different reversal sequences at both sides (one resulting in a fixed
        // point with multiple values, the other in irregular growth). Both also result in varying
        // loop starts.
        //
        //   *   * *
        // * o _ o _ *
        // * _ * o o
        // o _ o o *
        // o * _ o *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_MULTIPLE_VALUES);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::IRREGULAR_GROWTH);
    }
    SECTION( "6x6-TurnWithHeavilyOscillatingInSweepValue" ) {
        // The turn at the left of the sequence is caused by a zero, which is converted into a one.
        // However, during the transition an in-sequence 1 is also changed. It is changed as
        // follows: 1 => 0 => 2 => 1 => 2, after which the right-sweep starts
        //
        //       * *
        // *   * _ o *
        // o _ _ o o *
        // o _ * _ _ *
        // o * o _ *
        // _   * *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithDecreasingInSweepValue" ) {
        // Both the leftward and rightward sweep change the sequence value, but these cancel each
        // other out. The leftmost value, however, is only impacted by one of the sweeps, as the
        // transition at the left shifts DP so that the rightward sweep does not undo this change.
        //
        //   *     *
        //   _ _ * _
        // * _ _ _ o *
        // * o o o _ *
        // * * * _ _
        // o o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithTwoInSequenceOscillatingValues" ) {
        // Another sweep with a mid-sequence reversal that takes 21 steps to execute. As this
        // reversal sequence contains a small loop (with fixed number of iteration) it requires the
        // Sweep Hang detector to distinguish these fixed loops from the actual sweep loops. The
        // transition modifies two in-sequence values. These briefly change from -1 to -2 and back
        // again.
        //
        //       *
        //   * * o _ *
        // * _ _ o *
        // * o o o _ *
        // * * _ _ _ *
        // o _ o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepWithIncreasingMidSweepPoint" ) {
        // This is a sweep where the mid-sweep point is incremented by one during reversal. It
        // triggers a reversal as all other values covered by the sweep are -1. So the left sweep
        // loop is exited by a non-zero value condition.
        //
        //     * * *
        //   * _ o o *
        //   * _ _ *
        // * o o _ _ *
        // *   * o _
        // o o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP,
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_INCREASING_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithIncreasingMidSweepPoint2" ) {
        // Similar to the previous in behaviour
        //
        //       *
        //   * * _ *
        // * _ o _ o *
        //   _ _ _ *
        // * * o o
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_INCREASING_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithIncreasingMidSweepPoint3" ) {
        // Similar to the previous two programs, but more complex. Here mid-sweep point is
        // incremented by two during reversal. The sweep sequence consists of only -2 values. The
        // left-sweeping loop is complex: SHL 1, SHR 1, DEC 1, SHL 1, INC 2, DEC 1.
        //
        //     * * *
        //   * _ o o *
        //   * o _ *
        // * o _ o _ *
        // * o * o o
        // o o *   *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_INCREASING_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithTwoFixedConstantValueTurningPoints" ) {
        // Sweep with two fixed turning points. One sweep loops moves two data cells each iteration.
        // Its starting position on the data tape (modulus two) determines at which of these two
        // fixed points the sweep ends.
        //
        //     * * *
        //   * _ o o *
        //   _ _ _ *
        // * o o o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithTwoFixedIncreasingValueTurningPoints" ) {
        // Program that is similar to the previous one. However, the two fixed position turning
        // points do not have a constant value, but a continuously increasing value.
        //
        //     * * *
        //   * _ o o *
        //   * o _ *
        // * o _ _ _ *
        // o o * o _
        // _       *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_INCREASING_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithTwoFixedIncreasingValueTurningPoints2") {
        // Program very similar in structure and behavior to the previous.
        //
        //     * * *
        //   * _ _ o *
        //   * o o *
        // * o _ _ _ *
        // * o * o _
        // o o *   *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalDetectedHangs() == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_INCREASING_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-LateSweepWithMidsweepPoint" ) {
        // Program runs for 142 steps before it enters sweep hang.
        //
        //   *   *
        // * o * _ _ *
        // o o * o _
        // o _ _ o *
        // o * _ o _ *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-LateSweepWithOscillatingSweepValues") {
        // Sweep hang eventually sweeps across entire sequence (which contains both positive and
        // negative values). It takes about 100 steps before it enters the sweep hang. One sweep
        // includes a DEC 2 and INC 2 statement, which cancel each other out, but do cause values
        // to oscillate around zero.
        //
        //       *
        //     * o _ *
        // *   o o *
        // o o o o *
        // o * _ o _ *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-LateFullSweepWithDecreasingSweepValues") {
        // Sweep hang starts after about 170 steps. It sweeps all values and one of the sweep loops
        // decrements all values in the sweep sequence by one.
        //
        //       * *
        //   * _ _ _ *
        // * o o _ _ *
        // o o * o o *
        // o * _ o *
        // o     *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-RegularSweepWithOnlyLoops") {
        // A regular sweep which only consists of two sweep loops. When switching from one loop to
        // the other, there are no fixed instructions between either transition. The right-moving
        // sweep is also complex. The loop consists of seven instructions, two of which have a
        // zero continuation instruction.
        //
        //   *   *
        // * _ o o _ *
        //   _ * o _
        //   _ o o *
        // * _ _ o
        // o _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_INCREASING_VALUE);
    }
    SECTION( "6x6-SweepHangWithSweepDeltasBothWays" ) {
        // A single-headed sweep, where both sweeps decrease the sweep values by one.
        //
        //     * * *
        //   * _ o o *
        //   * o _ o
        //   _ _ _ *
        // * o _ o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::NOOP,
            Ins::DATA, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWithSweepDeltasBothWays2" ) {
        // Similar in behaviour to the previous program. It generates a cleaner sequence though.
        // The 2L program and PP execution path is quite pretty.
        //
        //   *
        //   _ _ * *
        // * _ _ _ o *
        // * o o _ _ *
        // * * * _ o
        // o o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWithSweepDeltasBothWays3" ) {
        // Similar to the previous two programs, but here the sequence extends leftwards.
        //
        //   *   * *
        // * o _ _ _ *
        //   _ * o o *
        // * o _ o *
        // * * _ o _ *
        // o _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
    }
    SECTION( "6x6-SweepHangWithSkippedLoopExit" ) {
        // The right-sweep exits when it encounters value 1, which happens to be the value of the
        // fixed end-point at the left. This, however, does not stop the sweep, as the transition
        // skips passed this before the right-sweep loop starts. On top of that, the right loop
        // decreases all values by one. This causes the left-sweep scan to abort when it encounters
        // the 1 at the end of the sweep. Even though it does not directly exit the loop, it
        // reasons it will become a zero.
        //
        //         *
        //   * _ _ _
        //   o _ * _
        // * o o _ o *
        // * * * _ o
        // o _ _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWhereTransitionUndoesSweepLoopChange" ) {
        // The left sweep decreases all values in the sweep sequence by one. The transition at the
        // left, however, undoes this change for one of the changes in the sequence. This prevented
        // an earlier, more strict, version of the hang detector to detect the hang.
        //
        //   *     *
        //   _ _ * o *
        // * _ _ o o *
        // * o o _ o *
        // * * * _ _
        // o _ _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithSignMismatchForExtensionAndCombinedSweepValueChange" ) {
        // Dual-headed sweep. At the right, the sweep is extended with value 1. This has a
        // different sign than the combined sweep value change, which is -2. However, this is okay,
        // as it is realized in one instruction, and therefore does not cause a zero-based exit.
        //
        // *     *
        // o _ * o _ *
        // _ _ _ o *
        // _ o _ o *
        // _ * _ o _ *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithSignMismatchForExtensionAndCombinedSweepValueChange2" ) {
        // Similar in behaviour to the previous program, but this time a the extension at the left
        // (value -1) has a different sign than the combined sweep hang change (2).
        //
        //       * *
        // * o _ o _ *
        // o _ _ o _ *
        // o * * o _
        // o * _ o *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithSignMismatchForExtensionAndCombinedSweepValueChange3") {
        // Similar to the previous two programs, but this time the rightwards sweep moves DP two
        // cells, which means that its -2 delta change is not uniform (as half the cells are
        // skipped).
        //
        //       *
        //   * * o _ *
        //   _ o o *
        //   _ _ o
        // * _ _ o
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithUniformChangesThatCancelEachOtherOut" ) {
        // The sweep extends to the right with value -2. The leftward sweep decreases all values
        // (skipping this one), and the right sweep increases all values. This means that this
        // "non-exit" value is never converted to an exit for the right sweep, and that the
        // sequence is growing steadily. An earlier version of the hang detector failed to detect
        // this, because it did not correctly take into account the bootstrapping of the sweep loop.
        //
        //   *     *
        //   _ _ * _
        // * _ _ o o *
        // * o o o _ *
        // * * * _ o
        // o _ o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithUniformChangesThatCancelEachOtherOut2" ) {
        // An earlier version of the sweep hang detector failed to detect the hang, as in
        // identifying possible exits for Loop #1, it did not take into account the uniform change
        // made by Loop #0 when scanning the sweep sequence.
        //
        //       *
        //       o _ *
        //     * o _
        // * _ _ o *
        // * * _ o
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithUniformChangesThatCancelEachOtherOut3" ) {
        // Similar to previous program.
        //
        // *     *
        // o _ * o _ *
        // _ _ * o _
        // _ o _ o *
        // _ * _ o
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SteadyGrowthSweepWithInSequenceExit") {
        // Dual-headed sweep with a complex transition at the right end. The right side of the
        // sequence consists of two 1's. The left-most one terminates the sweep. It is then
        // converted to a -1, extending the sequence body. Furthermore, it adds a new 1 value right
        // of the other one.
        //
        //       *
        //   * * o _ *
        // * _ o o *
        // * o _ o
        // * * _ o
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SteadyGrowthSweepWithInSequenceExit2") {
        // Similar in behaviour to previous program, but with a much more complicated transition
        // sequence. The transition extends the sequence as follows:
        // .. -1 1 2 =>
        // .. -2 1 2 =>
        // .. -2 0 2 =>
        // .. -2 0 2 2 =>
        // .. -2 -3 2 2 =>
        // .. -2 -2 2 2 =>
        // .. -2 -2 0 2 =>
        // .. -2 -1 0 2 =>
        // .. -2 -1 1 2
        //
        //       *
        //     * o _ *
        // * _ _ o *
        //   _ o o *
        // * * _ o _ *
        // o _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SteadyGrowthSweepWithInSequenceExit3" ) {
        // The sequence extension at the right has similar complexity as that of the previous
        // program. It also extends the sequence in eight steps:
        // .. 2 1 1 =>
        // .. 2 0 1 =>
        // .. 2 1 1 =>
        // .. 3 1 1 =>
        // .. 3 1 0 =>
        // .. 3 1 1 =>
        // .. 3 2 1 =>
        // .. 3 2 1 1
        //
        //     * * *
        //   * o o _ *
        //   o o o *
        //   _ o _ o *
        // * _ _ o _ *
        // o o * * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWithComplexFastGrowingEnd" ) {
        // This dual-headed sequence grows on the right side with two data cells each sweep. It
        // does so with a complex transition, which features two short fixed loops. This performs
        // the following changes:
        // -1  2  1  1       =>
        // -1  2  0  1 (loop exit) =>
        // -1  2  0  2       =>
        // -1  2  0  2  1    =>
        // -1  2 -2  2  1    =>
        // -1  2 -1  2  1    =>
        // -1  2 -1  1  1    =>
        // -1  2 -1  2  1    =>
        // -1  2 -1  2  0    =>
        // -1  2 -1  2  0  1 =>
        // -1  2 -1  2  1  1
        //
        //     *
        //   * o _ *
        //   _ o * *
        // * o o o o *
        // * _ _ o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWithComplexFastGrowingEnd2" ) {
        // Rightward sweep extends sequence with three values each iteration.
        //
        //       *
        //   * * o _ *
        //   _ o o *
        //   _ o o *
        // * _ _ o _ *
        // o _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWithComplexFastGrowingEnd3" ) {
        // The sequence is always extended with two values each iteration, but the values it adds
        // vary. It eithers adds 0 1 or 1 1. Interestingly, when it adds two ones, it also fills
        // up the zero-gap left in the previous extension so that the result at the right is a
        // growing sequence of ones. The steady growth on its left is also realized via a pretty
        // complex transition.
        //
        //     * * *
        //   * o o _ *
        //   o o o *
        // * o _ o _
        // * _ _ o _ *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWithMidSweepLoopSwitchWithoutTransition1" ) {
        // Sweep hang where the rightwards sweep features two sweep loops. The sweep is dual-headed.
        // The left side is extended with -1 values, the right side with 1's. The negative values
        // are decreased by one by the rightwards sweep, whereas the 1 values remain at one using
        // a more-complex variant of the loop.
        //
        //     * * *
        //   * o o _ *
        // * _ o o *
        //   _ * o o *
        // * o _ _ _ *
        // o o * * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWithMidSweepLoopSwitchWithoutTransition2" ) {
        // Simpler version of the previous program with very similar behavior.
        //
        //       *
        //     * o _ *
        //     o o *
        // * * _ o o *
        // o o _ * *
        // _   *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWithMidSweepLoopSwitchWithoutTransition3" ) {
        // Detection fails during proof due to flawed only zeroes ahead check. Should not be hard
        // to fix.
        //
        //       * *
        //   * * _ o *
        // * o o _ *
        // o o _ o *
        // o * o _ *
        // o   * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWithMidSweepLoopSwitchViaTransition" ) {
        // The rightwards sweep has a mid-sweep loop switch. Both loops are even separated by a
        // transition sequence, which shifts DP one position to the right. Analysis needs to take
        // this delta into account, as this skips past a value that causes the incoming loop to
        // exit.
        //
        //       *
        // *   * o _ *
        // o _ o o *
        // _ * _ o o *
        // _   _ * *
        // _   *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWithMidSweepLoopSwitchViaTransition2" ) {
        // This is a more complex program. The right sweep consists of two loops. The outgoing loop
        // (#7) moves DP two cells, which impacts the transition between both loops. Half of the
        // time the outgoing loop transfers immediately to the incoming loop (#14). The other times
        // there is a fixed transition (#26) after which it enters incoming loop #33, which is
        // rotation-equivalent to #14.
        //
        //       *
        //   * * o _ *
        //   _ o o *
        //   _ _ _ o *
        // * _ _ o _ *
        // o o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWithMidSweepLoopSwitchViaTransitionWithLoop" ) {
        // The rightwards sweep switches loops. Both loops are separated by a transition sequence
        // which itself contains a short loop, with a fixed number of iterations.
        //
        //       *
        // *   * o _ *
        // o o o o *
        // _ * _ o o *
        // _   _ * *
        // _   *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-DualHeadedSweepHangWithFastGrowingHead" ) {
        // Program with a complex sweep. The sequence consists of both positive and negative values.
        // During the sweep, the sign of some values oscillate (1 => DEC 2 => -1 => INC 2 => 1).
        // Finally, the transition at the right side is complex. It extends the sequence with three
        // values each visit.
        //
        // It currently fails due to improved indirect exit detection. What happens is:
        // a) The right sweep exits on zero, and leaves a 1 behind
        // b) The (in-sweep) neighbour to its left is changed from a zero to a -1, a delta of -1
        // From this it concludes that 1 is an indirect exit. This in isolation is true, however,
        // for this program it will never happen, as the in-sweep left neighbour will always be a
        // zero. This, however, is a bit awkward to proof.
        //
        //       *
        //   * * o _ *
        //   o o o *
        //   o   o *
        // * _ _ o _ *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-LateDualHeadedSweepWithFastGrowingHead2" ) {
        // Sweep that requires about 200 iterations to start. It then creates a dual-headed
        // sequence, with the right side growing with three values each iteration.
        //
        // Currently fails detection for same reason as previous program.
        //
        //       *
        //   * * o _ *
        //   o o o *
        //   o o o *
        // * _ _ o _ *
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-DualHeadedSweepHangWithFastGrowingHead3" ) {
        // Similar in behavior to the previous two programs. This one extends the sequence on the
        // right with "-2 2 2" after each sweep.
        //
        //       *
        //     * o _ *
        //   * * o *
        //   o o o *
        // * _ _ o _ *
        // o _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepHangWithComplexTransitionAtBothEnds" ) {
        // Sweep with fairly complex transition sequence at both ends, which also shifts DP. This
        // requires careful checking when scanning the sequence and/or checking if pre-conditions
        // are met.
        //
        //     * * *
        //   * o o _ *
        //   o o o *
        //   o _ o o *
        // * _ _ _ _ *
        // o o * * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
    SECTION( "6x6-SweepWithFixedPointMultiValueExitAndInSweepOscillatingChange" ) {
        // The right sweep ends at a fixed point with multiple (pairs of) values. It either ends
        // with 2 1, or 3 0. Analysis wrongly concluded that the sweep will be broken (as one
        // transition results in a delta of 2, which is bigger and opposite to the sweep value
        // change of -1).
        //
        //     * * *
        //   * o o _ *
        // * _ o o *
        //   _ * o _
        // * o _ _ _ *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_POINT_MULTIPLE_VALUES);
    }
    SECTION( "6x6-SweepWithFixedPointMultiValueExitAndInSweepOscillatingChange2" ) {
        // Similar in behavior to the previous program. Here, the sequence at the right either ends
        // with 0 -1 or -1 -1.
        //
        //       * *
        //   * * _ o *
        // * o _ _ *
        // o o o o *
        // - * o _ *
        // _   * *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::IRREGULAR_GROWTH);
    }
    SECTION( "6x6-SweepHangWithGhostInSweepTransitionDelta" ) {
        // The sweep transition at the left side is fairly complicated, which caused it to initially
        // fail hang detection. The output the program produces is actually simple:
        //   0 0 1 X 1 1 .... 1 1 0 0
        // The value X is increasing by one each sweep iteration. The right-side of the sequence
        // is extended with a 1-value each iteration. However, detection initially failed because
        // the transition at the right has a fixed-loop, whose last instruction is actually also
        // part of the next loop (the actual sweep loop). This loop does not modify the sequence.
        // However, due to how the run-block assignment works, it sees the following happening to
        // the value to the right of the value X
        // 1) Sweep transition decreases value by 1 (by the instruction that is actually part of
        //    the outgoing sweep loop)
        // 2) Due to bootstrap effects of the outgoing loop, this change is undone
        // So the value remains unchanged. However, it sees a transition delta of -1, combined with
        // the delta of the value X of 1, this failed an earlier version of the hang detector.
        //
        // ? ? * * ? ?
        // ? * o o _ *
        // ? o _ o * ?
        // ? o * _ _ ?
        // * _ _ o _ *
        // o o * * ? ?
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_POINT_CONSTANT_VALUE);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
}
