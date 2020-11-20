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

    SECTION( "6x6-IrregularGrowthFailing") {
        // Sweep with irregular growth at its left side. It alternates between two reversal
        // sequences:
        //
        // "... 0 0 1 1 [body]" => "... 0 1 0 3 [body]"
        // "... 0 0 1 0 [body]" => "... 0 0 1 1 [body]"
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

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
    SECTION( "6x6-SteadyGrowthSweepWithInSequenceExit3" ) {
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

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
    SECTION( "6x6-SweepHangWithTwoSweepLoopsInSameDirection2" ) {
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

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(tracker.getTotalDetectedHangs() == 0);
    }
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
}
