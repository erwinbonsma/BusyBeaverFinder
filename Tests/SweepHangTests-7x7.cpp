//
//  SweepHangTests-7x7.cpp
//  Tests
//
//  Created by Erwin on 20/12/2020.
//  Copyright © 2020 Erwin Bonsma.
//

#include "catch.hpp"

#include "HangExecutor.h"
#include "SweepHangDetector.h"

TEST_CASE("7x7 sweep hangs", "[hang][7x7][sweep][regular]") {
    HangExecutor hangExecutor(16384, 100000);
    hangExecutor.setMaxSteps(100000);
    hangExecutor.addDefaultHangDetectors();

    SECTION("7x7-SweepWithOscillatingBody") {
        // Sweep hang, where the sweep body oscillates each sweep. After the rightward sweep it
        // consists of only zeroes. After the leftward sweep it consists of alternativing zeros and
        // ones.
        //
        //     *   *
        // * _ o o o _ *
        // o _ _ _ _ *
        // _ _ _ _ o *
        // _ * _ _ _ _
        // _ * o _ o o *
        // _   *   *
        RunResult result = hangExecutor.execute("dwiCFSQCABggAJFgiA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::REGULAR_SWEEP);
    }
}

TEST_CASE("7x7 irregular sweep hangs", "[hang][7x7][sweep][irregular]") {
    HangExecutor hangExecutor(16384, 100000);
    hangExecutor.setMaxSteps(100000);
    hangExecutor.addDefaultHangDetectors();

    SECTION("7x7-IrregularSweep1") {
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
        RunResult result = hangExecutor.execute("d4CBKSAWAlgRgIYAIA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);

        REQUIRE(leftSweepEndType(hangExecutor) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(hangExecutor) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
}
