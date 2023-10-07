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
    HangExecutor hangExecutor(1024, 1000000);
    hangExecutor.setMaxSteps(1000000);

    SECTION("6x6-SweepWithInSequenceOscillatingZeros") {
        // The body of the sweep consists of alternating -1 and 0 values. The left side is a fixed
        // but increasing value, the right side of the sequence steadily grows. The leftward sweep
        // inverts the entire sweep body. The -1 values become zeroes, and vice versa. Even though
        // zero is an exit of the rightward sweep, this sweep moves DP two positions and is always
        // aligned such that it uses the -1 values as stepping stones.
        //
        //       * *
        //   * * o _ *
        // * _ o o _ *
        // * _ _ _ _ *
        // * * o _ *
        // o o o *
        RunResult result = hangExecutor.execute("ZgKCkoUoAqSFYA");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
    SECTION("6x6-SweepWithInSequenceOscillatingValues") {
        // Similar to the previous program, but here the sequence body consists of -1 and -2
        // values. The leftward sweep swaps these values every sweep.
        //
        //     * * *
        //   * _ o o *
        //   * o o *
        // * o _ _ _ *
        // *   * o o
        // o _ _ o *
        RunResult result = hangExecutor.execute("ZgqCFiWJAolEGA");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
    SECTION("6x6-SweepWithIrregularFixedPointGrowingValue") {
        // The right end of the sequence looks as follows: [body] 1 X 1 0 0 0
        // The rightward sweep loop moves DP two units. Half of the sweeps it ends on the first 1.
        // In that case, the loop exits and returns without making any modifications. The other
        // half of the sweeps it ends on the second 1. In that case, it increases the positive
        // value X by one.
        //
        //       *
        //   * * o _ *
        // * _ o o *
        // * o o _ _
        // * * _ o _ *
        // o _ o *
        RunResult result = hangExecutor.execute("ZgICkoWJQKEkYA");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::DATA_ERROR);
    }
    SECTION("6x6-LateStartingPeriodicSweepWithTwoFastGrowingEnds") {
        // This program executes a complex sweep that looks to be irregular but seems to becomes
        // regular eventually. TODO: Check why the meta-run summary does not reflect this.
        //
        //     * * *
        //   * o o _ *
        //   * * o _
        //   _ o o *
        // * _ _ o _ *
        // o o o *
        RunResult result = hangExecutor.execute("ZgqCUikAWIElYA");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::DATA_ERROR);
    }
}

TEST_CASE("6x6 Failing Irregular Sweep Hang tests", "[hang][sweep][irregular][6x6][fail]") {
    HangExecutor hangExecutor(1024, 20000);
    hangExecutor.setMaxSteps(1000000);

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

    SECTION("7x7-UndetectedHang1") {
        // *   *   * *
        // o _ _ _ _ _ *
        // _   _ * _ _
        // _ * o o o o *
        // _   * o o o *
        // _ * _ o o *
        // _   * * *
        RunResult result = hangExecutor.execute("d4ihACAgCVYJWIWAqA");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
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
    SECTION("7x7-UndetectedHang4") {
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

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
}

TEST_CASE("7x7 false positives", "[success][7x7][fail]") {
    HangExecutor hangExecutor(16384, 100000);
    hangExecutor.setMaxSteps(100000);

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
    SECTION("7x7-FalsePositive3") {
        // Program exhibits a behavior that resembles a simple hang. Steady growth at its right,
        // and a fixed point at its left. However, this fixed point is classified incorrectly. The
        // transition sequence (plus subsequent bootstrap of the outgoing loop) actually moves one
        // cell towards zero.
        //
        // Fix: Detect that transition sequence (with subsequent loop bootstrap) moves one cell
        // towards zero.
        //
        //   *     * *
        //   _ _ * o _ *
        // * o _ o o o *
        // * * * o * o
        // o o o o * o
        // _ * _ o _ o
        // _     *   *
        RunResult result = hangExecutor.execute("dyCgCSkVqmRVkIRAIg");

        // TEMP: Should not actually be detected as hanging
        REQUIRE(result == RunResult::DETECTED_HANG);
    }
    SECTION("7x7-FalsePositive4") {
        // Program exhibits a behavior that resembles a simple hang. Steady growth at both sides.
        // However, there is an isolated one at the left of the sequence that breaks the leftward
        // growth and causes the program to terminate.
        //
        // More specifically, the 1 is an exit value for the rightwards sweep, but the current
        // check does not check for this as it initiates hang detection when the sweep is at the
        // right side. It only scans the data with the leftwards sweep loop analysis, and correctly
        // establishes that 1 is not an exit condition for it.
        //
        // Fix: Correctly detect that this 1 value causes a different execution path.
        //
        //       *
        //     * o _ *
        //     * o * _ *
        // *   _ o * o
        // o o o o o o *
        // _ * _ _ _ o
        // _     *   *
        RunResult result = hangExecutor.execute("dwIAJICYoGRVWIBAIg");

        // TEMP: Should not actually be detected as hanging
        REQUIRE(result == RunResult::DETECTED_HANG);
    }
    SECTION("7x7-FalsePositive5") {
        // Very similar in behavior to the previous program.
        //
        // Fix: Same as previous
        //
        //   *   * *
        // * o o _ _ *
        //     * o o *
        // *   _ o * _ *
        // o o o o o o *
        // _ * _ o o o *
        // _   * *   *
        RunResult result = hangExecutor.execute("dyKCUICWIGJVWIVgog");

        // TEMP: Should not actually be detected as hanging
        REQUIRE(result == RunResult::DETECTED_HANG);
    }
    SECTION("7x7-FalsePositive6") {
        // Very similar in behavior to the previous program. Possibly a bit more subtle.
        //
        //       *
        //     * o _ _ *
        //     * o   *
        // *   _ o * _
        // o o o o o o *
        // _ * _ _ _ o
        // _     *   *
        RunResult result = hangExecutor.execute("dwIAJCCSIGBVWIBAIg");

        // TEMP: Should not actually be detected as hanging
        REQUIRE(result == RunResult::DETECTED_HANG);
    }
}
