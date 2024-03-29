//
//  IrregularSweepHangTests.cpp
//  Tests
//
//  Created by Erwin on 30/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "HangExecutor.h"
#include "SweepHangDetector.h"

TEST_CASE("6x6 Irregular Sweep Hang tests", "[hang][sweep][irregular][6x6]") {
    HangExecutor hangExecutor(1024, 20000);
    hangExecutor.setMaxSteps(1000000);
    hangExecutor.addDefaultHangDetectors();

    SECTION("6x6-IrregularSweepWhereIncomingLoopClears") {
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
        RunResult result = hangExecutor.execute("ZgqCUgWJGIYFgA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);

        REQUIRE(leftSweepEndType(hangExecutor) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(hangExecutor) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
    SECTION("6x6-IrregularSweepWhereIncomingLoopClears2") {
        // Very similar in behavior to the previous two programs. Also an appendix at its right,
        // with the same exit and non-exit values.
        //
        //       *
        //       o _ *
        // *   * o o
        // o _ _ o *
        // o * _ o *
        // o     *
        RunResult result = hangExecutor.execute("ZgIAEolEGGGEIA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);

        REQUIRE(leftSweepEndType(hangExecutor) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(hangExecutor) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
    SECTION("6x6-IrregularSweepWhereIncomingLoopClears3") {
        // The behavior of this program is very similar to the previous two programs, but reversed.
        // The appendix is on its left, and the sign of the appendix values is inverted.
        //
        //     *
        //   * o _ *
        //   * o _
        //   o o *
        // * _ o
        // o o *
        RunResult result = hangExecutor.execute("ZggCSCQBYIQFgA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);

        REQUIRE(leftSweepEndType(hangExecutor) == SweepEndType::FIXED_APERIODIC_APPENDIX);
        REQUIRE(rightSweepEndType(hangExecutor) == SweepEndType::STEADY_GROWTH);
    }
    SECTION("6x6-IrregularSweepWhereIncomingLoopOscillates") {
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
        RunResult result = hangExecutor.execute("ZgqCUhWIWIYFgA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);

        REQUIRE(leftSweepEndType(hangExecutor) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(hangExecutor) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
    SECTION("6x6-IrregularSweepWhereIncomingLoopOscillates2") {
        // Very similar to the previous two hanging programs. It was incorrectly classified as a
        // meta-periodic hang by an earlier verion of the sweep hang detection.
        //
        //     * * *
        //   * o o _ *
        //   * o _ *
        //   o o o *
        // * _ o o *
        // o o * *
        RunResult result = hangExecutor.execute("ZgqCUiSBWIWFoA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);

        REQUIRE(leftSweepEndType(hangExecutor) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(hangExecutor) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
    SECTION("6x6-IrregularSweepWhereIncomingLoopOscillates3") {
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
        RunResult result = hangExecutor.execute("ZgoCQiWBWIWFoA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);

        REQUIRE(leftSweepEndType(hangExecutor) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(hangExecutor) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
    SECTION("6x6-IrregularSweepHangWithPollutedAppendix") {
        // This sweep hang has an a-periodically growing appendix at its left. The exit value is
        // -1, the non-exit value is -2. However, the appendix is polluted by a positive value
        // that increased each time the incoming sweep loop passes it.
        //
        //       *
        //     * o _ *
        //     * o _
        //     o o *
        // * * _ o
        // o o o *
        RunResult result = hangExecutor.execute("ZgIAkgkAWKEFYA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);

        REQUIRE(leftSweepEndType(hangExecutor) == SweepEndType::FIXED_APERIODIC_APPENDIX);
        REQUIRE(rightSweepEndType(hangExecutor) == SweepEndType::STEADY_GROWTH);
    }
    SECTION("6x6-IrregularSweepHangWithPollutedAppendix2") {
        // This is very similar in behavior to the previous program.
        //
        //       *
        //     * o _ *
        // *   * o _
        // o o o o *
        // o * _ o
        // o     *
        RunResult result = hangExecutor.execute("ZgIAkokFWGEEIA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);

        REQUIRE(leftSweepEndType(hangExecutor) == SweepEndType::FIXED_APERIODIC_APPENDIX);
        REQUIRE(rightSweepEndType(hangExecutor) == SweepEndType::STEADY_GROWTH);
    }
    SECTION("6x6-IrregularSweepHangWithPollutedAppendix3") {
        // The sweep hang has an a-periodically growing appendix at its right side. The exit value
        // is 1, the non-exit 2. The appendix is polluted with a negative value that is decremented
        // each time it is passed by the incoming loop.
        //
        //   *   *
        // * o * o _ *
        // o o * o *
        // _ _ _ o *
        // _ * _ o _ *
        // _     *
        RunResult result = hangExecutor.execute("ZiIJklmAGCEgIA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);

        REQUIRE(leftSweepEndType(hangExecutor) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(hangExecutor) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
    SECTION("6x6-IrregularSweepHangWithPollutedAppendix4") {
        // Sweep hang with a relatively complex shifts by the sweep loops. The incoming loop to the
        // irregular appendix includes a SHR 2 and a SHL so that DP still moves only one unit each
        // iteration. The outgoing loop is similar as it includes a SHL2 and SHR but is more simple
        // as it does not make any modifications.
        //
        //     * *
        //   * o o _ *
        // * _ o o *
        //   _ * _
        // * _ o o *
        // o o o *
        RunResult result = hangExecutor.execute("ZgoCUoWAgIWFYA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::IRREGULAR_SWEEP);

        REQUIRE(leftSweepEndType(hangExecutor) == SweepEndType::STEADY_GROWTH);
        REQUIRE(rightSweepEndType(hangExecutor) == SweepEndType::FIXED_APERIODIC_APPENDIX);
    }
}
