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

    SECTION("6x6-IrregularSweepWithZeroesInAppendix") {
        // A truly binary counter. It actually uses ones and zeros, and also properly generates
        // binary numbers (only with most-significant bit at the right).
        //
        //   *   * *
        // * o _ _ _ *
        // o o * o o *
        // o   * o *
        // _ * _ o *
        // _     *
        RunResult result = hangExecutor.execute("ZiKJAllkmCGAIA");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::DATA_ERROR);
    }
}

TEST_CASE("6x6 Failing Irregular Other Hangs", "[hang][irregular][6x6][fail]") {
    HangExecutor hangExecutor(1024, 20000);
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
        RunResult result = hangExecutor.execute("ZgKCkpUoAqWEYA");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
    SECTION("6x6-IrregularSweepHangWithHeavilyPollutedAppendix") {
        // A non-standard irregular sweep. The leftward sweep loop moves DP two cells and creates
        // a heavily polluted a-periodically growing appendix. The sweep loops exits on -1 cells,
        // which it then converts to the limbo value -2, which the sweep can convert to the -1
        // value again. In between pairs of -1/-2 values are postives values. Interestingly, their
        // value decrease exponentially. After approximately 2.25 M steps: 200 > 100 > 50 > 26 >
        // 13 > 7 > 4 > 2.
        //
        //     *
        //   * o _ _ *
        //   * o o _
        //   o o * *
        // * _ o _ *
        // o o *
        RunResult result = hangExecutor.execute("ZggCQiUBaISFgA");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::DATA_ERROR);
    }
}

TEST_CASE("7x7 undetected hangs", "[hang][7x7][fail]") {
    HangExecutor hangExecutor(16384, 100000);
    hangExecutor.setMaxSteps(100000);
    hangExecutor.addDefaultHangDetectors();

    SECTION("7x7-UndetectedHang2") {
        // Irregular sweep.
        //
        // TODO: Analyze why hang is not detected with current logic.
        //
        // *       *
        // o _ * _ o _ *
        // _ _ * * o _
        // _ _ o o _ *
        // _ o _ * o _
        // _ * * _ o o *
        // _       * *
        RunResult result = hangExecutor.execute("d4CBISCkAUgSQKFgCg");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
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

    SECTION("7x7-FalsePositive1") {
        // Program exhibits a behavior that resembles an irregular sweep hang, with an aperiodic
        // appendix at its right. However, the leftwards sweep moves DP by two cells each
        // iteration. All leftward sweeps, however, seem to end on the first zero at the left. This
        // is caused by how the "bits" in the binary appendix at the right toggle. However, after
        // 2000 steps the left sweep lands on the second zero, which causes the program to exit.
        //
        // Fix: Do not allow sweep loop that moves away from aperiodic appendix to move DP more
        // than one?
        //
        // *     * *
        // o _ * _ _ _ *
        // _ _ * o o _
        // _ _ o o o *
        // _ o _ o _ *
        // _ * _ o o *
        // _   * * *
        RunResult result = hangExecutor.execute("d4KBICCUAVgRIIWAqA");

        // TEMP: Should not actually be detected as hanging
        REQUIRE(result == RunResult::DETECTED_HANG);
    }
    SECTION("7x7-FalsePositive2") {
        // Program exhibits a behavior that resembles an irregular sweep hang, with an aperiodic
        // appendix at its right. However, the sequence that changes a -1 bit in the binary
        // appendix to a -2 requires that the cell located two cells to its left has a non-zero
        // value. This is not the case for a -1 value near the end of the appendix.
        //
        // Fix: Require that in-appendix transition sequences for aperiodic appendix do not inspect
        // values beyond the bit that they are toggling.
        //
        //     *     *
        //   * _ _ * o *
        // * _ o o o _ *
        // *   _ * o _
        // o o o * o *
        // _   * _ o *
        // _       *
        RunResult result = hangExecutor.execute("dwgggmhUoJBWYCGACA");

        // TEMP: Should not actually be detected as hanging
        REQUIRE(result == RunResult::DETECTED_HANG);
    }
}
