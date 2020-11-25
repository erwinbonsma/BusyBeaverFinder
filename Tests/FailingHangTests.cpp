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

    SECTION( "6x6-SweepHangWithTwoSweepLoopsInSameDirection3" ) {
        // This is a more complex program. The right sweep consists of two loops. The outgoing loop
        // (#7) moves DP two cells, which impacts the transition between both loops. Half of the
        // time the outgoing loop transfers immediately to the incoming loop (#14). The other times
        // there is a fixed transition (#26) after which it enters incoming loop #33, which is
        // rotation-equivalent to #14.
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
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

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
//        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);
//
//        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
//        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
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

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);

//        REQUIRE(tracker.getTotalHangs(HangType::REGULAR_SWEEP) == 1);
//
//        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
//        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
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

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
    SECTION( "6x6-SweepHangWithMidSweepLoopSwitchWithoutTransition" ) {
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
            Ins::TURN,  Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
    SECTION( "6x6-SweepHangWithMidSweepLoopSwitchViaTransition" ) {
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

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
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
}
