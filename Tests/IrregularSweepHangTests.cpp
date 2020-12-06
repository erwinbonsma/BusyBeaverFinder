//
//  IrregularSweepHangTests.cpp
//  Tests
//
//  Created by Erwin on 30/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "ExhaustiveSearcher.h"
#include "SweepHangDetector.h"

TEST_CASE( "6x6 Irregular Sweep Hang tests", "[hang][sweep][irregular][6x6]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectionSteps = 20000;
    settings.maxSteps = 1000000;
    searcher.configure(settings);

    SECTION( "6x6-IrregularSweep2") {
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

        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_APERIODIC_APPENDIX);
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

        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::FIXED_APERIODIC_APPENDIX);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
    }
}
