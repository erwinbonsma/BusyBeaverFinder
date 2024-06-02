//
//  SweepHangTests-7x7.cpp
//  Tests
//
//  Created by Erwin on 20/12/2020.
//  Copyright © 2020 Erwin Bonsma.
//

#include "catch.hpp"

#include "HangExecutor.h"

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

    SECTION("7x7-ComplexIrregularSweep1") {
        // This program executes an irregular sweep where the left side grow regularly by two
        // units each sweep (both -2) which extends the sweep body that consists of only negative
        // values. At the right is a sweep appendix that consists of positive values. The
        // rightward sweep subtracts one from each value. The sweep terminates when it encounters
        // a one. What this exit value, E, becomes depends on its right neighbour, R:
        // R = 0 => E = 3,         R = 1
        // R > 0 => E = 3 + R - 1, R = 2
        // This is achieved by executing a small loop.
        //
        // Example appendix growth:
        // 3 5 2 1
        // 2 4 1 3 1
        // 1 3 5 2 1
        // 5 2 5 2 1
        // 4 1 4 1 3 1
        // 3 6 2 1 3 1
        //
        // The values in the appendix do not become large very rapidly. After a million steps (and
        // a little over 400 sweeps) the appendix is as follows:
        // 10 1 14 2 2 2 2 2 2 2 5 2 2 1 2 3 1
        //
        // *       *
        // o _ * * o _ *
        // _ _   o o *
        // _ _ * o o *
        // _ o _ o *
        // _ * _ o *
        // _     *
        RunResult result = hangExecutor.execute("d4CBKSAWAlgRgIYAIA");

        REQUIRE(result == RunResult::ASSUMED_HANG);
//        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);

//        REQUIRE(leftSweepEndType(hangExecutor) == SweepEndType::STEADY_GROWTH);
//        REQUIRE(rightSweepEndType(hangExecutor) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
    SECTION("7x7-UndetectedHang2") {
        // Irregular sweep.
        //
        // *       *
        // o _ * _ o _ *
        // _ _ * * o _
        // _ _ o o _ *
        // _ o _ * o _
        // _ * * _ o o *
        // _       * *
        RunResult result = hangExecutor.execute("d4CBISCkAUgSQKFgCg");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);
    }
}
