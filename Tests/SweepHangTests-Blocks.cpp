//
//  SweepHangTests-Blocks.cpp
//  Tests
//
//  Created by Erwin on 26/02/2024.
//  Copyright © 2024 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "HangExecutor.h"
#include "SweepHangDetector.h"

const int dummySteps = 1;
const int maxSequenceLen = 16;

const bool INC = true;
const bool MOV = false;

TEST_CASE("Block-based Sweep Hang Tests", "[hang][sweep][blocks]") {
    HangExecutor hangExecutor(1000, 20000);
    hangExecutor.setMaxSteps(20000);
    hangExecutor.addDefaultHangDetectors();

    ProgramBlock block[maxSequenceLen];
    for (int i = 0; i < maxSequenceLen; i++) {
        block[i].init(i);
    }
    ProgramBlock *exitBlock = &block[maxSequenceLen - 1];
    exitBlock->finalizeExit(dummySteps);

    SECTION("SweepLooksBeyondSweepExit") {
        // The right sweep looks beyond the zero value that will cause it to exit, which must be
        // non-zero. After the sweep terminates, the sweep extends one position to the right,
        // again with a zero followed by a one. The sweep also extends one position to the left
        // each time. Together this ensures that look-ahead is always non-zero.

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  2, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, block + 4, exitBlock);

        // Left sweep
        block[4].finalize(MOV, -1, dummySteps, block + 5, block + 4);

        // Extend at left
        block[5].finalize(INC,  1, dummySteps, exitBlock, block + 6);

        // Right sweep
        block[6].finalize(MOV,  3, dummySteps, exitBlock, block + 7); // Look ahead, non-zero
        block[7].finalize(MOV, -1, dummySteps, block + 8, block + 6); // Sweep exit

        // Extend at right, preserving invariant
        block[ 8].finalize(INC,  1, dummySteps, exitBlock, block + 9);
        block[ 9].finalize(MOV,  1, dummySteps, exitBlock, block + 10);
        block[10].finalize(INC, -1, dummySteps, block + 11, exitBlock);
        block[11].finalize(MOV,  1, dummySteps, block + 12, exitBlock);
        block[12].finalize(INC,  1, dummySteps, exitBlock, block + 13);
        block[13].finalize(MOV, -2, dummySteps, exitBlock, block + 4);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::REGULAR_SWEEP);
    }

    SECTION("SweepWithTwoStationaryEndTransitions") {
        // Copy of program in SweepAnalysis tests
        //
        // The sweep extends to the left by one unit each time. The rightward sweep moves one
        // unit and increments all cells during its traversal. It always exits at the same
        // loop instruction, but alternates between two different data positions. The subsequent
        // transition sequences diverge to restore the sweep's tail so that it keeps alternating
        // between both sequences. The tail ends are respectively A = [... -1 1 0] and
        // B = [... -2 -1 0].
        //
        // Correctly determining that this program hangs requires properly analyzing the combined
        // impact of the right sweep (which modifies a sentinel position) and the two transition
        // sequences at the right.

        // Bootstrap
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV, -1, dummySteps, block + 2, exitBlock);
        block[2].finalize(INC, -1, dummySteps, exitBlock, block + 3);

        // Leftward sweep
        block[3].finalize(MOV, -1, dummySteps, block + 4, block + 3);

        // Transition - extends sequence at left
        block[4].finalize(INC, 1, dummySteps, exitBlock, block + 5);

        // Rightward sweep
        block[5].finalize(INC, 1, dummySteps, block + 7, block + 6);
        block[6].finalize(MOV, 1, dummySteps, exitBlock, block + 5);

        // Transition sequence (right)
        block[7].finalize(MOV, 1, dummySteps, block + 11, block + 8);

        // Transition A: ... -1 1 0 => ... 0 [1] 0 => ... -2 -1 0
        block[ 8].finalize(INC, -2, dummySteps, exitBlock, block + 9);
        block[ 9].finalize(MOV, -1, dummySteps, block + 10, exitBlock);
        block[10].finalize(INC, -2, dummySteps, exitBlock, block + 3);

        // Transition B: ... -2 -1 0 => ... -1 0 [0] => ... -1 1 0
        block[11].finalize(MOV, -1, dummySteps, block + 12, exitBlock);
        block[12].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::REGULAR_SWEEP);
    }

    SECTION("SweepConsumesRemoteContamination") {
        // Sweep that consumes a remote contamination, and does not end.
        // See also: SweepTerminatesOnRemoteContamination

        // Bootstrap
        block[0].finalize(INC, -2, dummySteps, exitBlock, block + 1); // Near-miss sentinel
        block[1].finalize(MOV, -16, dummySteps, block + 5, exitBlock);

        // Rightwards sweep
        block[2].finalize(MOV,  1, dummySteps, block + 5, block + 3); // Normal exit
        block[3].finalize(INC,  1, dummySteps, exitBlock, block + 4); // Hang-breaking exit
        block[4].finalize(INC, -1, dummySteps, exitBlock, block + 2);

        // Transition sequence, extending sequence by one
        block[5].finalize(INC,  1, dummySteps, exitBlock, block + 6);

        // Leftwards sweep
        block[6].finalize(MOV, -1, dummySteps, block + 2, block + 6);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::REGULAR_SWEEP);
    }

    SECTION("StripedSweepSkipsRemoteContamination") {
        // Sweep that skips over a remote contamination and does not end.
        // See also: StripedSweepTerminatesOnRemoteContamination

        // Bootstrap
        block[0].finalize(INC, -1, dummySteps, exitBlock, block + 1); // Sentinel
        block[1].finalize(MOV, -17, dummySteps, block + 2, exitBlock);

        // Rightwards sweep
        block[2].finalize(MOV,  2, dummySteps, block + 5, block + 3); // Normal exit
        block[3].finalize(INC,  1, dummySteps, exitBlock, block + 4); // Hang-breaking exit
        block[4].finalize(INC, -1, dummySteps, exitBlock, block + 2);

        // Transition sequence, extending sequence by two
        block[5].finalize(INC,  1, dummySteps, exitBlock, block + 6);

        // Leftwards sweep
        block[6].finalize(MOV, -2, dummySteps, block + 2, block + 6);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::DETECTED_HANG);
        REQUIRE(hangExecutor.detectedHangType() == HangType::REGULAR_SWEEP);
    }
}

TEST_CASE("Block-based Sweep Completion Tests", "[success][sweep][blocks]") {
    HangExecutor hangExecutor(1000, 20000);
    hangExecutor.setMaxSteps(20000);
    hangExecutor.addDefaultHangDetectors();

    ProgramBlock block[maxSequenceLen];
    for (int i = 0; i < maxSequenceLen; i++) {
        block[i].init(i);
    }
    ProgramBlock *exitBlock = &block[maxSequenceLen - 1];
    exitBlock->finalizeExit(dummySteps);

    SECTION("TerminatingSweepWithStripedBody") {
        // The rightward sweep is a loop that moved DP two positions. One cell it decrements, the
        // other is constant and terminates the sweep. However, there's a counter at the right
        // which causes the sweep loop to eventually exit abnormally, breaking the hang.

        // Bootstrap
        block[0].finalize(INC, 16, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV, -1, dummySteps, block + 7, exitBlock);

        // Rightwards sweep
        block[3].finalize(INC, -1, dummySteps, exitBlock, block + 4);
        block[4].finalize(MOV,  1, dummySteps, block + 6, block + 5);
        block[5].finalize(MOV,  1, dummySteps, exitBlock, block + 3);

        // Leftwards sweep
        block[6].finalize(MOV, -1, dummySteps, block + 7, block + 6);

        // Transition sequence extending sequence at left
        block[7].finalize(INC,  1, dummySteps, exitBlock, block + 8);
        block[8].finalize(MOV, -1, dummySteps, block + 9, exitBlock);
        block[9].finalize(INC, -1, dummySteps, exitBlock, block + 3);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::SUCCESS);
    }

    SECTION("TerminatingSweepByPrematureExitWithSideeffects") {
        // Modified version of ConstantSweepBodyWithStationaryCounter2 in SweepAnalysisTests.
        // There's a counter at the left that is incremented by one each meta-loop iteration.
        // This is a result of the premature exit of the left sweep. It causes this program to
        // eventually terminate.

        // Bootstrap
        block[0].finalize(INC, -16, dummySteps, exitBlock, block + 1);

        // Rightwards sweep
        block[1].finalize(MOV,  1, dummySteps, block + 2, block + 1);

        // Transition sequence that extends sequence
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        // Leftwards sweep, which oscilates each value in the sweep body during traversal
        block[3].finalize(INC,  1, dummySteps, exitBlock, block + 4);
        block[4].finalize(MOV, -1, dummySteps, block + 1, block + 5);
        block[5].finalize(MOV,  1, dummySteps, exitBlock, block + 6);
        block[6].finalize(INC, -1, dummySteps, exitBlock, block + 7);
        block[7].finalize(MOV, -1, dummySteps, exitBlock, block + 3);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::SUCCESS);
    }

    SECTION("SweepTerminationOnMidSweepTransition") {
        // Sweep with a mid-sweep transition, which decreases a counter and terminates when this
        // counter reaches zero.

        // Bootstrap
        block[0].finalize(INC, 16, dummySteps, exitBlock, block + 1);

        // Rightwards sweep
        block[1].finalize(MOV,  1, dummySteps, block + 2, block + 1);

        // Transition sequence, extending sequence by one
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);

        // Leftwards sweep (on right side)
        block[3].finalize(INC, -1, dummySteps, block + 4, block + 6);
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);
        block[5].finalize(MOV, -1, dummySteps, exitBlock, block + 3);

        // Leftwards sweep (on left side)
        block[6].finalize(MOV, -1, dummySteps, block + 7, block + 6);

        // Transition sequence extending sequence at left
        block[7].finalize(INC,  1, dummySteps, exitBlock, block + 1);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::SUCCESS);
    }

    SECTION("SweepTerminatesOnRemoteContamination") {
        // Sweep that terminates because there are not only zeroes ahead.

        // Bootstrap
        block[0].finalize(INC, -1, dummySteps, exitBlock, block + 1); // Sentinel
        block[1].finalize(MOV, -16, dummySteps, block + 5, exitBlock);

        // Rightwards sweep
        block[2].finalize(MOV,  1, dummySteps, block + 5, block + 3); // Normal exit
        block[3].finalize(INC,  1, dummySteps, exitBlock, block + 4); // Hang-breaking exit
        block[4].finalize(INC, -1, dummySteps, exitBlock, block + 2);

        // Transition sequence, extending sequence by one
        block[5].finalize(INC,  1, dummySteps, exitBlock, block + 6);

        // Leftwards sweep
        block[6].finalize(MOV, -1, dummySteps, block + 2, block + 6);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::SUCCESS);
    }

    SECTION("StripedSweepTerminatesOnRemoteContamination") {
        // Variant of StripedSweepSkipsRemoteContamination
        // This one terminates because the sentinel is offset differently and not skipped as a
        // result

        // Bootstrap
        block[0].finalize(INC, -1, dummySteps, exitBlock, block + 1); // Sentinel
        block[1].finalize(MOV, -16, dummySteps, block + 2, exitBlock);

        // Rightwards sweep
        block[2].finalize(MOV,  2, dummySteps, block + 5, block + 3); // Normal exit
        block[3].finalize(INC,  1, dummySteps, exitBlock, block + 4); // Hang-breaking exit
        block[4].finalize(INC, -1, dummySteps, exitBlock, block + 2);

        // Transition sequence, extending sequence by two
        block[5].finalize(INC,  1, dummySteps, exitBlock, block + 6);

        // Leftwards sweep
        block[6].finalize(MOV, -2, dummySteps, block + 2, block + 6);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::SUCCESS);
    }

    SECTION("StripedSweepTerminatesOnRemoteContamination2") {
        // Variant of previous program.
        // This one terminates because the sweep examines all values, and the sentinel breaks
        // the hang as it is non-zero.

        // Bootstrap
        block[0].finalize(INC, 3, dummySteps, exitBlock, block + 1); // Sentinel
        block[1].finalize(MOV, -17, dummySteps, block + 2, exitBlock);

        // Rightwards sweep
        block[2].finalize(MOV,  1, dummySteps, block + 3, exitBlock); // Hang-breaking exit
        block[3].finalize(MOV,  1, dummySteps, block + 4, block + 2); // Normal exit

        // Transition sequence, extending sequence by two
        block[4].finalize(INC,  1, dummySteps, exitBlock, block + 5);

        // Leftwards sweep
        block[5].finalize(MOV, -2, dummySteps, block + 2, block + 5);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        RunResult result = hangExecutor.execute(&program);

        REQUIRE(result == RunResult::SUCCESS);
    }
}
