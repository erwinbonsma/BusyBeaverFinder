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

TEST_CASE( "6x6 Failing Hang tests", "[hang][6x6][.fail]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxPeriodicHangDetectAttempts = 4;
    settings.initialHangSamplePeriod = 16;
    searcher.configure(settings);

    SECTION( "6x6-RegularSweepWithDoubleShiftFailing" ) {
        // This program has two noteworthy features:
        // - Its mid-sequence turning point does not have a fixed value. It briefly becomes zero,
        //   then becomes non-zero again.
        // - When sweeping leftwards, it shifts left twice, which causes the strict regular hang
        //   detection to fail (as skipped values could possibly have an impact)
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
    }
    SECTION( "6x6-IrregularSweep") {
        // An irregular sweep. Its turning point at one end of the sequence varies. The reason is
        // that it shifts DP two positions, which means it can ignore a mid-sequence zero.
        //
        //     * * *
        //   * o o _ *
        //   o o o *
        //   _ o _
        // * _ _ o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);
    }
    SECTION( "6x6-IrregularSweep2") {
        // Another irregular sweep.
        //
        //       *
        //   * * o _ *
        //   o o o *
        // * _ _ o *
        // * _   *
        // o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);
    }
    SECTION( "6x6-IrregularSweep3") {
        // Another irregular sweep.
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

        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);
    }
    SECTION( "6x6-IrregularSweep4" ) {
        // An irregular sweep that was wrongly found by an early version of the Regular Sweep Hang
        // detector. Moving right, it shifts DP two positions each time. As a result, it skips over
        // some values. Initially, the sequence seems to have balanced grow, but it is broken up by
        // zeroes appearing mid-sequence, and the execution becomes quite chaotic.
        //
        //   *   * *
        // * o o o _ *
        //   * * o _
        //   - o o *
        // * _ _ o *
        // o _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 0);
        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);
    }
    SECTION( "6x6-Glider1") {
        // A non-periodic hang where two ever-increasing values move rightward on the tape, leaving
        // zeroes in their wake. The amount of steps required to move one position on the tape
        // doubles each time (as the left counter is decreased towards zero, the right counter is
        // increased by two).
        //
        //     * *
        //   * _ _ o *
        //   o _ o o *
        //   _ * o o
        // * _ o o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::APERIODIC_GLIDER) == 1);
    }
    SECTION( "6x6-Glider2" ) {
        // A glider that moves leftwards.
        //
        //     *
        // *   o o *
        // o _ _ _ *
        // o * o o _ *
        // o   _ *
        // o   *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::APERIODIC_GLIDER) == 1);
    }
    SECTION( "6x6-Glider3" ) {
        // Another leftward moving glider.
        //
        //     *   *
        // * o o o _ *
        // o _ _ _ _ *
        // o _ o * _
        // o * _ o o *
        // _   *   *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::APERIODIC_GLIDER) == 1);
    }
    SECTION( "6x6-Glider4" ) {
        // A glider that was wrongly found by an early version of the Regular Sweep Hang detector.
        //
        //     * *
        // *   o o *
        // o _ o _ *
        // o * o o _ *
        // o * o *
        // o   *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 0);
        REQUIRE(tracker.getTotalHangs(HangType::APERIODIC_GLIDER) == 1);
    }
    SECTION( "6x6-Glider5" ) {
        // A glider that was wrongly found by an early version of the Regular Sweep Hang detector.
        //
        //   *   *
        // * o o _ *
        // * o _ o _ *
        // * _ o * _
        // * * * o o *
        // o _ _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 0);
        REQUIRE(tracker.getTotalHangs(HangType::APERIODIC_GLIDER) == 1);
    }
    SECTION( "6x6-ComplexGlider" ) {
        // Another leftward moving glider, but with relatively complex logic
        //
        //   * *   *
        // * o o o _ *
        // o _ _ * o
        // o _ o * o
        // o * _ _ o *
        // o   *   *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::APERIODIC_GLIDER) == 1);
    }
    SECTION( "6x6-MidSweepLeftTurn" ) {
        // This program contains a mid-sweep left turn. It is caused by a mid-sequence one, which
        // is decreased to zero. It then is increased twice and decreased once and back at the
        // location where it carried out the left turn. Now, as the value is one again, it
        // continues the sweep loop with a right turn.
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

        // TODO: Classify this hang
        REQUIRE(tracker.getTotalDetectedHangs() == 1);
    }
    SECTION( "6x6-SweepExceedingBoundaries" ) {
        // When reversing the sweep at the left side, DP performs another shift, visiting another
        // zero value. This breaks the current assumption that the next turn cannot be further than
        // the previous sweep end point.
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

        REQUIRE(tracker.getTotalHangs() == 1);
    }
}
