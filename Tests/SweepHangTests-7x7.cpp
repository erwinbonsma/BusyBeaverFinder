//
//  SweepHangTests-7x7.cpp
//  Tests
//
//  Created by Erwin on 20/12/2020.
//  Copyright © 2020 Erwin Bonsma.
//

#include "catch.hpp"

#include "ExhaustiveSearcher.h"
#include "SweepHangDetector.h"

TEST_CASE( "7x7 sweep hangs", "[hang][7x7][sweep][irregular]" ) {
    ExhaustiveSearcher searcher(7, 7, 16384);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectionSteps = 100000;
    settings.maxSteps = settings.maxHangDetectionSteps;
    settings.undoCapacity = settings.maxSteps;
    searcher.configure(settings);

    SECTION( "7x7-IrregularSweep1" ) {
        // Sweep detected after 7480 steps. The transition at the right is fairly complex. It's
        // an aperiodic appendix (to be confirmed after more detailed analysis) where the
        // transition sequence can include itself loops of varying length. This is something the
        // current detector does not support. Nevertheless, it does classify it as an irregular
        // sweep.
        //
        // TODO: Analyze further. Is there a risk of false-positive detection for similar programs?
        //
        // *       *
        // o _ * * o _ *
        // _ _   o o *
        // _ _ * o o *
        // _ o _ o *
        // _ * _ o *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
}
