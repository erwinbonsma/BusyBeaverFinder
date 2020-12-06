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

    SECTION( "6x6-SweepWithFixedPointMultiValueExitAndInSweepOscillatingChange" ) {
        // The right sweep ends at a fixed point with multiple (pairs of) values. It either ends
        // with 2 1, or 3 0. Analysis wrongly concludes that the sweep will be broken (due to
        // 3 being changed to 2 - to be confirmed).
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

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
    SECTION( "6x6-SweepWithFixedPointMultiValueExitAndInSweepOscillatingChange2" ) {
        // Similar in behavior to the previous program. Here, the sequence at the right either ends
        // with 0 -1 or -1 -1.
        //
        //       * *
        //   * * _ o *
        // * o _ _ *
        // o o o o *
        // o * o _ *
        // _   * *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
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
}

TEST_CASE( "6x6 Failing Irregular Sweep Hang tests", "[hang][sweep][irregular][6x6][fail]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectionSteps = 20000;
    settings.maxSteps = 1000000;
    searcher.configure(settings);

    SECTION( "6x6-IrregularSweep1") {
        // Another irregular sweep.
        //
        // It has a meta-meta-run loop.
        // The meta-run is as follows: #10(9) #9[12 14 7 16 ...]
        // The sweep loops are 12 and 7. Meta-loop #9 also has an increasing number of iterations.
        //
        //     * * *
        //   * o o _ *
        //   o o o *
        // * _ o o *
        // * _ o *
        // o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalDetectedHangs() == 0); // TEMP
    }
    SECTION( "6x6-SweepWithBinaryCounter2") {
        // Another sweep with a binary-like counter.
        //
        //     * * *
        //   * o o _ *
        //   o o o *
        //   _ o _
        // * _ o o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalDetectedHangs() == 0); // TEMP
    }
    SECTION( "6x6-SweepWithBinaryCounter3") {
        // Very similar to previous hang. However, it has an interesting feature that next to the
        // binary counter, inside the sweep sequence, is a value that is decreased only when the
        // left-most binary counter bit flips. So it effectively counts the number of bits of the
        // binary counter.
        //     * *
        //   * o _ _ *
        //   * o o *
        //   o o o *
        // * _ o o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalDetectedHangs() == 0); // TEMP
    }
    SECTION( "6x6-SweepWithBinaryCounter5" ) {
        // Another irregular sweep that was incorrectly classified as a meta-periodic hang.
        //
        //     * * *
        //   * o o _ *
        //   * o _ *
        //   o o o *
        // * _ o o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalDetectedHangs() == 0); // TEMP
    }
    SECTION( "6x6-SweepWithBinaryCounter6" ) {
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
            Ins::TURN, Ins::TURN,  Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
}
