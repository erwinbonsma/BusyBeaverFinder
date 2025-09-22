//
//  FailingHangTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 17/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "catch.hpp"

#include "HangExecutor.h"

TEST_CASE("6x6 Failing Hang tests", "[hang][regular][sweep][6x6][fail]") {
    HangExecutor hangExecutor(1024, 100000);
    hangExecutor.setMaxSteps(100000);
    hangExecutor.addDefaultHangDetectors();
}

TEST_CASE("6x6 Failing Irregular Sweep Hang tests", "[hang][sweep][irregular][6x6][fail]") {
    HangExecutor hangExecutor(1024, 20000);
    hangExecutor.setMaxSteps(1000000);
    hangExecutor.addDefaultHangDetectors();

    // No known failures yet
}

TEST_CASE("6x6 Failing Irregular Other Hangs", "[hang][irregular][6x6][fail]") {
    HangExecutor hangExecutor(2048, 20000);
    hangExecutor.setMaxSteps(1000000);
    hangExecutor.addDefaultHangDetectors();

    SECTION("6x6-IrregularHopScotch") {
        // This program executes a curious sweep. The sweep does not have a sweep body. At the left
        // there's a fixed, ever-increasing exit. Directly attached is an a-periodically growing
        // appendix which consists of three values: 0, -1 and -2.
        //
        // The right-moving loop moves DP two steps. It increases the values it lands on by one,
        // converting -2's to -1's. It exits on zero.
        //
        // The left-moving sweep is more chaotic. It clears -1 values and leaves -2 values behind.
        // It only converts half of the -1 values to -2. The others it resets to zero.
        //
        //       * *
        //   * * o _ *
        // * o o o _ *
        // * _ _ _ _ *
        // * * o o *
        // o _ o *
        RunResult result = hangExecutor.execute("Zv6+kpUoAqW0bw");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
    SECTION("6x6-DualEndedIrregularSweep") {
        // Sweep hang with an irregularly growing appendix at both ends.
        //
        // On top of that, the behavior at the right side is unusual. Here, the in-sweep exit is
        // zero and the toggle value is one. It toggles values in the appendix when the sweep
        // returns, but only (about) half of the appendix values it traverses. It sets the
        // immediate neighbour to zero, and every second value in the appendix.
        //
        // The appendix at the left has an in-sweep exit of -1, and toggle value of -2. The sweep
        // body consists of two values that both move towards negative infinity.
        //
        // The data after 1000 steps is: ... 0 -2 -2 -2 -1 -4 -13 1 1 1 1 1 1 0 ...
        //
        //     * * *
        //   * o o _ *
        //   * o o o *
        // * _ o o *
        // * o * *
        // o o *
        RunResult result = hangExecutor.execute("Zvq+UuVoW5r1vw");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
    SECTION("6x6-BodylessIrregularSweep") {
        // This used to be detected, but not anymore since commit f0054bfc.
        //     * *
        //   * o o _ *
        //   _ _ o *
        // * _ _ o *
        // * o * *
        // o o *
        RunResult result = hangExecutor.execute("Zvr+UsG4G5r1vw");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
    SECTION("6x6-IrregularSweepWithAlternatingEndSweepTransition") {
        // The sweep hang has an irregular end at its right, consisting of zeroes and ones. The
        // rightward sweep ends on the first zero.
        //
        //   *   * *
        // * o o _ _ *
        // * o * o o *
        // o o * o *
        // o * _ o *
        // o     *
        //
        // The run-summary is as follows:
        // ...
        // #13* 5.0 #15                 #28* 5.1 #29
        // #13* 7.0 #15 #28*1.0 #35*1.0 #28* 5.1 #29
        // #13* 7.0 #15                 #28* 7.1 #29
        // #13*10.0 #15 #28*1.0 #35*2.0 #28* 7.1 #29
        // #13* 9.0 #15                 #28* 9.1 #29
        // #13*11.0 #15 #28*1.0 #35*1.0 #28* 9.1 #29
        // #13*11.0 #15                 #28*11.1 #29
        // #13*16.0 #15 #28*1.0 #35*4.0 #28*11.1 #29
        // #13*13.0 #15                 #28*13.1 #29
        //
        // There are two possible end-sweep transitions at the irregular end, and the hang
        // alternates between both. The first is a plain one: #15. The second is one that
        // contains a loop whose length varies depending on the amount of ones in the irregular
        // end: #15 #28*1.0 #35*n
        //
        // There is a mid-sweep transition, but only for half of the leftward sweeps.
        RunResult result = hangExecutor.execute("Zu65Qpllm2G37w");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
    SECTION("6x6-IrregularSweepWithZeroesInAppendix") {
        // A truly binary counter. It actually uses ones and zeros, and also properly generates
        // binary numbers (only with most-significant bit at the right).
        //
        // It is similar in behavior to 6x6-IrregularSweepWithAlternatingEndSweepTransition
        //
        //   *   * *
        // * o _ _ _ *
        // o o * o o *
        // o   * o *
        // _ * _ o *
        // _     *
        RunResult result = hangExecutor.execute("ZiKJAllkmCGAIA");

        hangExecutor.dumpExecutionState();

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
}

TEST_CASE("7x7 undetected hangs", "[hang][7x7][fail]") {
    HangExecutor hangExecutor(16384, 100000);
    hangExecutor.setMaxSteps(100000);
    hangExecutor.addDefaultHangDetectors();

    SECTION("7x7-UndetectedHang3") {
        // Program that causes Periodic Sweep Hang Detector to create transition-groups with
        // unique transitions. This caused an assertion to fail, which after analysis, has been
        // removed.
        //
        // *     * *
        // o _ _ _ _ _ *
        // _   * _ o _
        // _ * o o o *
        // _   _ o _
        // _ * _ _ _ o *
        // _   * * * *
        RunResult result = hangExecutor.execute("d4KBACCECVgBAIBgqg");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
}

TEST_CASE("7x7 false positives", "[success][7x7][fail]") {
    HangExecutor hangExecutor(16384, 100000);
    hangExecutor.setMaxSteps(100000);
    hangExecutor.addDefaultHangDetectors();

    // No known failures yet
}
