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

    SECTION( "6x6-IrregularSweepWhereIncomingLoopClears" ) {
        // Irregular sweep with an a-periodically growing appendix at its right end. The incoming
        // loop exits on 1 and then converts this value to the non-exit 2. The incoming loop also
        // resets all 2 values it passes back to 1.
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
    SECTION( "6x6-IrregularSweepWhereIncomingLoopClears2" ) {
        // Very similar in behavior to the previous two programs. Also an appendix at its right,
        // with the same exit and non-exit values.
        //
        //       *
        //       o _ *
        // *   * o o
        // o _ _ o *
        // o * _ o *
        // o     *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
    SECTION( "6x6-IrregularSweepWhereIncomingLoopClears3" ) {
        // The behavior of this program is very similar to the previous two programs, but reversed.
        // The appendix is on its left, and the sign of the appendix values is inverted.
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
    SECTION( "6x6-IrregularSweepWhereIncomingLoopOscillates" ) {
        // Exit value is 1, non-exit is 2. The incoming loop oscillates the non-exit value (2) to
        // the exit value (1) and restores it to the non-exit unless the loop was exited. In the
        // latter case the value that neighbours the exit remains at 1, and becomes the new exit.
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

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
    SECTION( "6x6-IrregularSweepWhereIncomingLoopOscillates2" ) {
        // Very similar to the previous two hanging programs. It was incorrectly classified as a
        // meta-periodic hang by an earlier verion of the sweep hang detection.
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

        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
    SECTION( "6x6-IrregularSweepWhereIncomingLoopOscillates3") {
        // Very similar to previous three irregular sweep hangs. However, it has an interesting
        // feature that next to the binary counter, inside the sweep sequence, is a value that is
        // decreased only when the left-most binary counter bit flips. So it effectively counts the
        // number of bits of the binary counter.
        //
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

        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);

        REQUIRE(leftSweepEndType(tracker) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(tracker) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
}
