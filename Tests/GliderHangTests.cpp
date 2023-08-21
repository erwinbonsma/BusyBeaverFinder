//
//  GliderHangTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "catch.hpp"

#include "HangExecutor.h"

TEST_CASE("6x6 Glider Hang tests", "[hang][glider][6x6]") {
    HangExecutor hangExecutor(1024, 1000000);
    hangExecutor.setMaxSteps(1000000);

    SECTION("6x6-Glider1") {
        // A non-periodic hang where two ever-increasing values move rightward on the tape, leaving
        // zeroes in their wake. The amount of steps required to move one position on the tape
        // doubles each time (as the left counter is decreased towards zero, the right counter is
        // increased by two).
        //
        //     * *
        //   * _ _ o *
        //   o _ o o *
        //   _ * o o
        // * _ o o *
        // o o * *
        RunResult result = hangExecutor.execute("66_xfecnblsonr");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
    SECTION("6x6-Glider2") {
        // A glider that moves leftwards.
        //
        //     *
        // *   o o *
        // o _ _ _ *
        // o * o o _ *
        // o   _ *
        // o   *
        RunResult result = hangExecutor.execute("66b_soifpkirk_");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
    SECTION("6x6-Glider3") {
        // Another leftward moving glider.
        //
        //     *   *
        // * o o o _ *
        // o _ _ _ _ *
        // o _ o * _
        // o * _ o o *
        // _   *   *
        RunResult result = hangExecutor.execute("66bfvkibjronbf");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
    SECTION("6x6-Glider4") {
        // A glider that was wrongly found by an early version of the Regular Sweep Hang detector.
        //
        //     * *
        // *   o o *
        // o _ o _ *
        // o * o o _ *
        // o * o *
        // o   *
        RunResult result = hangExecutor.execute("66brsojfpkprk_");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
    SECTION("6x6-Glider5") {
        // A glider that was wrongly found by an early version of the Regular Sweep Hang detector.
        //
        //   *   *
        // * o o _ *
        // * o _ o _ *
        // * _ o * _
        // * * * o o *
        // o _ _ o *
        RunResult result = hangExecutor.execute("66frvfuksrznio");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
    SECTION("6x6-Glider6") {
        // Glider Hang that is challenging to detect as the meta-run loop starts relatively late and
        // the iterations of glider loop increases exponentially. The program therefore runs for
        // many steps (~100000) before the hang is detected.
        //
        //     *   *
        // * o o o _ *
        // o _ _ o o *
        // o * _   o
        // _ * _ o o *
        // _   *   *
        RunResult result = hangExecutor.execute("66bfvkinocfnbf");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
    SECTION("6x6-Glider7") {
        // Another leftward moving glider, but with relatively complex logic
        //
        //   * *   *
        // * o o o _ *
        // o _ _ * o
        // o _ o * o
        // o * _ _ o *
        // o   *   *
        RunResult result = hangExecutor.execute("66hfvkiujuoekf");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
    SECTION("6x6-Glider8") {
        // A glider hang that is detected, but only very late (after 71140 steps). The reason it
        // takes this long is that the length of each loop triples each iteration and the detector
        // conservatively requires a few meta-loop iterations before concluding its a hang.
        //
        //     *   *
        // * o o o _ *
        // o _ o o _ *
        // o _ o * _
        // o * _ o o *
        // o   *   *
        RunResult result = hangExecutor.execute("66bfvkjkjronkf");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
    SECTION("6x6-Glider9") {
        // Leftward moving glider which in its transition sequence modifies a datacell in its wake.
        // It was not detected by the initial implementaton of the static Glider Hang detector,
        // which did not support these changes yet.
        //
        //   *     *
        //   o o * o *
        // * o o o _ *
        //   _ * _ *
        // * *   _
        // o _ o o *
        RunResult result = hangExecutor.execute("66ffdwvkbfx_jo");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
    SECTION("6x6-Glider10") {
        // A glider hang where the transition sequence consists of a fixed loop that always loops
        // twice.
        //
        //   * *   *
        // * o o o _ *
        // * o o o o *
        // * _ _ o *
        // * * _ *
        // o _ o *
        RunResult result = hangExecutor.execute("66hfvkvnroxrjr");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
    SECTION("6x6-Glider12") {
        // A glider hang where the transition sequence modifies values further ahead than the
        // next-next loop counter. This caused the old "only zeros ahead" check to fail.
        //
        //   * *   *
        // * o o o _ *
        // * o o o o *
        // * _ o o *
        // * * _ _
        // o _ o *
        RunResult result = hangExecutor.execute("66hfvkvnsox_jr");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
    SECTION("6x6-Glider13") {
        // A glider similar in behavior to Glider12, but with a different, pretty program.
        //   * *   *
        // * o o o _ *
        //   o o   _
        // * _ o   _
        // * * o _ o *
        // o _ o * *
        RunResult result = hangExecutor.execute("66hfvkd_s_yejx");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
    SECTION("6x6-GliderWithCounterQueue") {
        // Glider hang where the next counter in the glider loop is not immediately the current
        // counter in the next glider loop iteration. It takes two iterations of the glider loop
        // before it becomes the current counter. So effectively there is a queue of glider
        // counters.
        //
        //   *
        //   o o o *
        // * o o o _ *
        //   _ * *
        // * *   o o *
        // o _ _ o *
        RunResult result = hangExecutor.execute("66f_dovkbrxnio");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
}

TEST_CASE("7x7 Glider Hang tests", "[hang][glider][7x7]") {
    HangExecutor hangExecutor(1024, 1000000);
    hangExecutor.setMaxSteps(1000000);

    SECTION("7x7-GliderWithLargeRunBlocks") {
        // After a fairly chaotic start (which creates various one-off run blocks that contain many
        // program blocks) this program ends up in a glider hang. The glider hang itself also
        // consists of two large run blocks
        //
        // The loop      : 517 = 7 8 11 15    17 4 6 21
        //                       7 9 11 14 19 17 4 6 21
        //                       7 9 10 12       4 6 21
        // The transition: 596 =                     20
        //                       7 9 10 12       4 6 21
        //                       7 8 11 15    17 4 6 20
        //                       7 9 11 14 19 16
        //                       7 9 10 12       4 6 21
        //                       7 8 11 15 16
        //                       7 9 11 14 19 17 4 6 21 22
        //                           10 12 4 6       21
        //                       7 8 10 12 4 5
        //                       7 8 11 15    17 4 6 21
        //                       7 9 11 14 19 17 4 6 21
        //                       7 9 10 12       4 6 20
        //
        // Block #596 consists of 79 program blocks. Madness!
        // The total sequence tree consists of 597 program blocks.
        //
        //     *
        // * _ o _ * *
        //   * _ o o o *
        // * o o o _ *
        //   _ _ * o
        // * _ o * _ *
        // o o *   *
        RunResult result = hangExecutor.execute("77b_fkrsnvk_gbefnf_");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
}
