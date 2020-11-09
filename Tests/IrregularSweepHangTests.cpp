//
//  IrregularSweepHangTests.cpp
//  Tests
//
//  Created by Erwin on 30/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "ExhaustiveSearcher.h"

// Eventually the irregular hangs should be correctly detected. That can only happened when this
// is supported by a hang detector. That is not yet the case. Until then, let these test pass as
// long as they do not detect a hang. This guards against false positives in the current regular
// sweep hang detector.
TEST_CASE( "6x6 Irregular Sweep Hang tests", "[hang][sweep][irregular][6x6]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectionSteps = 20000;
    settings.maxSteps = 1000000;
    searcher.configure(settings);

    SECTION( "6x6-IrregularSweep3") {
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
    SECTION( "6x6-IrregularSweep5") {
        // Quite similar to 6x6-IrregularSweep3. However, the negative values in the data sequence
        // are updated differently. So useful to check if both can be detected by a more advanced
        // detection algorithm.
        //
        //     * * *
        //   * o o _ *
        //   _ o o *
        // * o _ o *
        // * _ o *
        // o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalDetectedHangs() == 0); // TEMP
    }
    SECTION( "6x6-SweepWithBinaryCounter") {
        // Sweep with binary counter at its left side. When the binary counter overflows, it adds
        // a bit and starts again at zero.
        //
        //     *
        //   * o _ *
        //   * o _
        //   o o *
        // * _ o
        // o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::UNSET
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
    SECTION( "6x6-SweepWithBinaryCounter4" ) {
        // Irregular sweep that was incorrectly classified as a meta-periodic hang.
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
}
