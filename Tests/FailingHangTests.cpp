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
    settings.maxSteps = 1000000;
    settings.maxHangDetectAttempts = 1024;
    settings.disableNoExitHangDetection = true;
    searcher.configure(settings);

    SECTION( "6x6-IrregularSweep") {
        // An irregular sweep. Its turning point at one end of the sequence varies. The reason is
        // that it shifts DP two positions, which means it can ignore a mid-sequence zero.
        //
        // It has a meta-run loop with period 16: [12 14 5 16 12 14 5 18 12 14 5 20 12 14 5 22 ..]
        // Here 12 and 5 are the sweep loops. At the right of the sequence it uses four different
        // reversal sequences: 16, 18, 20, and 22.
        //
        //     * * *
        //   * o o _ *
        //   o o o *
        //   _ o _
        // * _ _ o *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);
    }
    SECTION( "6x6-IrregularSweep2") {
        // Another irregular sweep.
        //
        // It has a meta-meta-run loop.
        // The meta-run is as follows: #15[13 1 2 11 13 1 2 11] #22[12 1 2 11 12 1 2 11]
        // Here 1 and 11 are the sweep loops.
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

        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);
    }
    SECTION( "6x6-IrregularSweep4" ) {
        // An irregular sweep that was wrongly found by an early version of the Regular Sweep Hang
        // detector. Moving right, it shifts DP two positions each time. As a result, it skips over
        // some values. Initially, the sequence seems to have balanced grow, but it is broken up by
        // zeroes appearing mid-sequence, and the execution becomes quite chaotic.
        //
        // The meta-run is as follows:
        // #17[24 27 24 27]
        // #10(11 3)
        // #28[12 15 16 3 24 27 12 15 16 3 24 27 12 15 16 3]
        // #34(12 15 16 3 11 3 12 15 20 3)
        // #17[24 27 24 27]
        // #38(11 3 12 15 28 3 11 3)
        // #28[12 15 16 3 24 27 12 15 16 3 24 27 12 15 16 3]
        // #34(12 15 16 3 11 3 12 15 20 3)
        // Here all meta-run loops are of fixed size.
        //
        // It features three base-level loops of varying size. However, their size is not strictly
        // increasing over time:
        // #3[79 61 ..]
        // #15[25 43 ..]
        // #27[61 79 ..]
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
}
