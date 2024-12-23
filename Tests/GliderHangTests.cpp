//
//  GliderHangTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "catch.hpp"

#include "HangExecutor.h"
#include "ProgramBlock.h"

const int dummySteps = 1;
const int maxSequenceLen = 24;

const bool INC = true;
const bool MOV = false;

TEST_CASE("6x6 Glider Hang tests", "[hang][glider][6x6]") {
    HangExecutor hangExecutor(1024, 1000000);
    hangExecutor.setMaxSteps(1000000);
    hangExecutor.addDefaultHangDetectors();

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
        RunResult result = hangExecutor.execute("ZgKCBhFglIWFoA");

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
        RunResult result = hangExecutor.execute("ZggIWECGUkIEgA");

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
        RunResult result = hangExecutor.execute("ZgiJUkAkYGFgiA");

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
        RunResult result = hangExecutor.execute("ZgoIWESGUmYEgA");

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
        RunResult result = hangExecutor.execute("ZiIJSJEoYKlkGA");

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
        RunResult result = hangExecutor.execute("ZgiJUkFmBCFgiA");

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
        RunResult result = hangExecutor.execute("ZiiJUkJEZGBkiA");

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
        RunResult result = hangExecutor.execute("ZgiJUkUkYGFkiA");

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
        RunResult result = hangExecutor.execute("ZiCBZpUgiKAEWA");

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
        RunResult result = hangExecutor.execute("ZiiJUpVoGKIEYA");

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
        RunResult result = hangExecutor.execute("ZiiJUpVoWKAEYA");

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
        RunResult result = hangExecutor.execute("ZiiJUhQIQKRkaA");

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
        RunResult result = hangExecutor.execute("ZiABWJUgoKFkGA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
}

TEST_CASE("7x7 Glider Hang tests", "[hang][glider][7x7]") {
    HangExecutor hangExecutor(1024, 1000000);
    hangExecutor.setMaxSteps(1000000);
    hangExecutor.addDefaultHangDetectors();

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
        RunResult result = hangExecutor.execute("dwgCEoIVpUgCQhiFiA");

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::APERIODIC_GLIDER);
    }
}

TEST_CASE("Block-based Glider Hang Tests", "[hang][glider][blocks]") {
    HangExecutor hangExecutor(1000, 20000);
    hangExecutor.setMaxSteps(20000);
    hangExecutor.addDefaultHangDetectors();

    ProgramBlock block[maxSequenceLen];
    for (int i = 0; i < maxSequenceLen; i++) {
        block[i].init(i);
    }
    ProgramBlock *exitBlock = &block[maxSequenceLen - 1];
    exitBlock->finalizeExit(dummySteps);

    SECTION("Glider-TransitionClearsZeroesFarAhead") {
        // This glider program has a transition which writes ones several steps ahead of its
        // glider loop. This complicates checks that there are only zeroes ahead.
        //
        // This is a variant of Glider-ExitsOnTransitionPolutions, which terminates on polutions
        // created by the transition. Here instead the polutions are required for the progrma to
        // hang.

        // Bootstrap Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  2, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, block + 4, exitBlock);
        block[4].finalize(INC, 10, dummySteps, exitBlock, block + 5);

        // Bootstrap Glider
        block[5].finalize(INC, -1, dummySteps, block + 9, block + 6);
        block[6].finalize(MOV, -1, dummySteps, exitBlock, block + 7);
        block[7].finalize(INC,  1, dummySteps, exitBlock, block + 8);
        block[8].finalize(MOV,  1, dummySteps, exitBlock, block + 5);

        // Bootstrap Transition
        block[ 9].finalize(INC,  1, dummySteps, exitBlock, block + 10);
        block[10].finalize(MOV, -1, dummySteps, exitBlock, block + 11);
        block[11].finalize(INC, -2, dummySteps, block + 18, block + 12);
        block[12].finalize(MOV, -1, dummySteps, block + 7, exitBlock);

        // Main Glider
        block[14].finalize(INC, -1, dummySteps, block + 18, block + 15);
        block[15].finalize(MOV,  1, dummySteps, exitBlock, block + 16);
        block[16].finalize(INC,  1, dummySteps, exitBlock, block + 17);
        block[17].finalize(MOV, -1, dummySteps, exitBlock, block + 14);

        // Main Transition
        block[18].finalize(MOV,  7, dummySteps, block + 19, exitBlock);
        block[19].finalize(INC,  2, dummySteps, exitBlock, block + 20); // far-ahead data set
        block[20].finalize(MOV, -5, dummySteps, exitBlock, block + 16);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::DETECTED_HANG);
    }
}


TEST_CASE("Block-based Glider Completion Tests", "[success][glider][blocks]") {
    HangExecutor hangExecutor(1000, 20000);
    hangExecutor.setMaxSteps(20000);
    hangExecutor.addDefaultHangDetectors();

    ProgramBlock block[maxSequenceLen];
    for (int i = 0; i < maxSequenceLen; i++) {
        block[i].init(i);
    }
    ProgramBlock *exitBlock = &block[maxSequenceLen - 1];
    exitBlock->finalizeExit(dummySteps);

    SECTION("Glider-TransitionExitsOnGliderCounter") {
        // This glider program terminates because the transition "checks" the next glider counter
        // and exits when its value is eight. A simple analysis might conclude that the program
        // hangs before that happens.

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV, -1, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        // Glider loop
        block[3].finalize(INC, -1, dummySteps, block + 7, block + 4);
        block[4].finalize(MOV,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(INC,  1, dummySteps, exitBlock, block + 6);
        block[6].finalize(MOV, -1, dummySteps, exitBlock, block + 3);

        // Transition
        block[7].finalize(MOV,  1, dummySteps, exitBlock, block + 8);
        block[8].finalize(INC, -8, dummySteps, exitBlock, block + 9);
        block[9].finalize(INC,  9, dummySteps, exitBlock, block + 3);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::SUCCESS);
    }

    SECTION("Glider-ExitsOnTransitionPolutions") {
        // This glider program has a transition which writes ones ahead of its glider loop.
        // When these are consumed, the glider loop exits

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV, -1, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        // Glider loop
        block[3].finalize(INC, -1, dummySteps, block + 7, block + 4);
        block[4].finalize(MOV,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(INC,  2, dummySteps, exitBlock, block + 6);
        block[6].finalize(MOV, -1, dummySteps, exitBlock, block + 3);

        // Transition
        block[ 7].finalize(MOV,  8, dummySteps, block + 8, exitBlock);
        block[ 8].finalize(INC,  1, dummySteps, exitBlock, block + 9);  // far-ahead data set
        block[ 9].finalize(MOV, -6, dummySteps, block + 10, exitBlock); // eventually fails
        block[10].finalize(INC,  1, dummySteps, exitBlock, block + 11); // clear zero
        block[11].finalize(MOV, -1, dummySteps, exitBlock, block + 3);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::SUCCESS);
    }
}
