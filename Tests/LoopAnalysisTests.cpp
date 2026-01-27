//
//  LoopAnalysisTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "Utils.h"
#include "LoopAnalysis.h"
#include "ProgramBlock.h"

constexpr int dummySteps = 1;

constexpr bool INC = true;
constexpr bool MOV = false;

TEST_CASE("Exit condition tests", "[loop-analysis]") {
    ExitCondition exitCondition;

    SECTION("ModuloGreaterThanExitCondition") {
        exitCondition.init(Operator::GREATER_THAN_OR_EQUAL, 3, 0);
        exitCondition.setModulusConstraint(3);

        REQUIRE(exitCondition.isTrueForValue(3));
        REQUIRE(exitCondition.isTrueForValue(6));
        REQUIRE(exitCondition.isTrueForValue(9));

        // Match modulus, but not comparison
        REQUIRE(!exitCondition.isTrueForValue(0));
        REQUIRE(!exitCondition.isTrueForValue(-3));

        // Match comparison, but not modulus
        REQUIRE(!exitCondition.isTrueForValue(4));
        REQUIRE(!exitCondition.isTrueForValue(5));

        // Match neither
        REQUIRE(!exitCondition.isTrueForValue(2));
    }
    SECTION("ModuloLessThanCondition") {
        exitCondition.init(Operator::LESS_THAN_OR_EQUAL, -2, 0);
        exitCondition.setModulusConstraint(5);

        REQUIRE(exitCondition.isTrueForValue(-2));
        REQUIRE(exitCondition.isTrueForValue(-7));
        REQUIRE(exitCondition.isTrueForValue(-12));

        // Match modulus, but not comparison
        REQUIRE(!exitCondition.isTrueForValue(3));
        REQUIRE(!exitCondition.isTrueForValue(8));

        // Match comparison, but not modulus
        REQUIRE(!exitCondition.isTrueForValue(-3));

        // Match neither
        REQUIRE(!exitCondition.isTrueForValue(0));
    }
}

void analyzeLoop(LoopAnalysis& la, ProgramBlock* startBlock, int numBlocks) {
    const ProgramBlock* blockP[maxLoopSize];

    for (int i = 0; i < numBlocks; ++i) {
        blockP[i] = startBlock++;
    }

    la.analyzeLoop(blockP, numBlocks);
}

TEST_CASE("Stationary loop classification tests", "[loop-analysis][stationary]") {
    ProgramBlock exitBlock{-1};

    auto program = create_indexed_array<ProgramBlock, maxLoopSize>();
    ProgramBlock *loopBlock = program.data();

    LoopAnalysis la;

    SECTION("StationarySingleChangeIncByOne") {
        loopBlock[0].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 1);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 1);
        REQUIRE(la.numBootstrapCycles() == 0);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -1));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
    }
    SECTION("StationarySingleChangeDecByThree") {
        loopBlock[0].finalize(INC, -3, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 1);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 1);
        REQUIRE(la.numBootstrapCycles() == 0);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::GREATER_THAN_OR_EQUAL, 3));
        REQUIRE(la.exit(0).exitCondition.modulusConstraintEquals(3));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
    }
    SECTION("StationarySingleChangeInTwoSteps") {
        loopBlock[0].finalize(INC, 2, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(INC, 3, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 2);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 1);
        REQUIRE(la.numBootstrapCycles() == 0);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -2));
        REQUIRE(la.exit(0).exitCondition.modulusConstraintEquals(5));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -5));
        REQUIRE(la.exit(1).exitCondition.modulusConstraintEquals(5));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
    }
    SECTION("StationarySingleChangeInTwoStepsWithBootstrap") {
        loopBlock[0].finalize(INC, 5, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(INC, -4, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 2);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 1);
        REQUIRE(la.numBootstrapCycles() == 4);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -5));
        REQUIRE(la.exit(0).exitCondition.modulusConstraintEquals(1));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -1));
        REQUIRE(la.exit(1).exitCondition.modulusConstraintEquals(1));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::BOOTSTRAP);
    }
    SECTION("StationarySingleChangeInTwoStepsWithBootstrapAndModulus") {
        loopBlock[0].finalize(INC, 4, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(INC, -2, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 2);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 1);
        REQUIRE(la.numBootstrapCycles() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -4));
        REQUIRE(la.exit(0).exitCondition.modulusConstraintEquals(2));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, -2));
        REQUIRE(la.exit(1).exitCondition.modulusConstraintEquals(2));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::BOOTSTRAP);
    }
    SECTION("StationarySingleChangeInTwoStepsWithBootstrapAndModulus2") {
        // As StationarySingleChangeInTwoStepsWithBootstrapAndModulus, but with longer bootstrap.
        // This also impacts the bootstrap operator.
        loopBlock[0].finalize(INC, 6, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(INC, -4, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 2);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 1);
        REQUIRE(la.numBootstrapCycles() == 2);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -6));
        REQUIRE(la.exit(0).exitCondition.modulusConstraintEquals(2));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -2));
        REQUIRE(la.exit(1).exitCondition.modulusConstraintEquals(2));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::BOOTSTRAP);
    }
    SECTION("StationarySingleChangeInThreeSteps") {
        loopBlock[0].finalize(INC, 5, dummySteps, &exitBlock, loopBlock + 1);  // INC 5
        loopBlock[1].finalize(INC, -2, dummySteps, &exitBlock, loopBlock + 2); // DEC 2
        loopBlock[2].finalize(INC, 2, dummySteps, &exitBlock, loopBlock + 0);  // INC 2

        analyzeLoop(la, loopBlock, 3);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 1);
        REQUIRE(la.numBootstrapCycles() == 0);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -5));
        REQUIRE(la.exit(0).exitCondition.modulusConstraintEquals(5));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -3));
        REQUIRE(la.exit(1).exitCondition.modulusConstraintEquals(5));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(2).exitWindow == ExitWindow::NEVER);
    }
    SECTION("StationarySingleChangeWithDummyShift") {
        loopBlock[0].finalize(MOV, 1, dummySteps, &exitBlock, loopBlock + 1);  // SHR
        loopBlock[1].finalize(MOV, -1, dummySteps, &exitBlock, loopBlock + 2); // SHL
        loopBlock[2].finalize(INC, 2, dummySteps, &exitBlock, loopBlock + 0);  // INC

        analyzeLoop(la, loopBlock, 3);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 1);
        REQUIRE(la.numBootstrapCycles() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(0).exitCondition.dpOffset() == 1);
        REQUIRE(la.exit(0).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(1).exitCondition.dpOffset() == 0);
        REQUIRE(la.exit(1).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -2));
        REQUIRE(la.exit(2).exitCondition.modulusConstraintEquals(2));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::ANYTIME);
    }
    SECTION("StationarySingleChangeWithDummyShiftAndNonZeroExit") {
        loopBlock[0].finalize(MOV, 1, dummySteps, loopBlock + 1, &exitBlock);  // SHR   # Expects 0
        loopBlock[1].finalize(MOV, -1, dummySteps, &exitBlock, loopBlock + 2); // SHL
        loopBlock[2].finalize(INC, 2, dummySteps, &exitBlock, loopBlock + 0);  // INC

        analyzeLoop(la, loopBlock, 3);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 1);
        REQUIRE(la.numBootstrapCycles() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::UNEQUAL, 0));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -2));
        REQUIRE(la.exit(2).exitCondition.modulusConstraintEquals(2));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::ANYTIME);
    }
    SECTION("StationaryTwoChanges") {
        loopBlock[0].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 1);  // INC
        loopBlock[1].finalize(MOV, 1, dummySteps, &exitBlock, loopBlock + 2);  // SHR
        loopBlock[2].finalize(INC, -2, dummySteps, &exitBlock, loopBlock + 3); // DEC 2
        loopBlock[3].finalize(MOV, -1, dummySteps, &exitBlock, loopBlock + 0); // SHL

        analyzeLoop(la, loopBlock, 4);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 2);
        REQUIRE(la.numBootstrapCycles() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::LESS_THAN_OR_EQUAL, -1));
        REQUIRE(la.exit(0).exitCondition.modulusConstraintEquals(1));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::GREATER_THAN_OR_EQUAL, 2));
        REQUIRE(la.exit(2).exitCondition.modulusConstraintEquals(2));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(3).exitWindow == ExitWindow::NEVER);
    }
    SECTION("StationaryOscillating") {
        loopBlock[0].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(INC, -1, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 2);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 0);
        REQUIRE(la.numBootstrapCycles() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::BOOTSTRAP);
    }
    SECTION("StationaryOscillatingWithNonZeroExit") {
        loopBlock[0].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 1);  // INC
        loopBlock[1].finalize(INC, -1, dummySteps, loopBlock + 0, &exitBlock); // DEC   # Expects 0

        analyzeLoop(la, loopBlock, 2);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 0);
        REQUIRE(la.numBootstrapCycles() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::UNEQUAL, 0));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::BOOTSTRAP);
    }
    SECTION("StationaryOscillatingWithNonZeroExit2") {
        // As StationaryOscillatingWithNonZeroExit, but with non-zero condition swapped
        loopBlock[0].finalize(INC, 1, dummySteps, loopBlock + 1, &exitBlock);  // INC   # Expects 0
        loopBlock[1].finalize(INC, -1, dummySteps, &exitBlock, loopBlock + 0); // DEC

        analyzeLoop(la, loopBlock, 2);

        REQUIRE(la.dataPointerDelta() == 0);
        REQUIRE(la.dataDeltas().size() == 0);
        REQUIRE(la.numBootstrapCycles() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::UNEQUAL, -1));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::NEVER);
    }
}

TEST_CASE("Travelling loop classification tests", "[loop-analysis][travelling]") {
    ProgramBlock exitBlock{-1};

    auto program = create_indexed_array<ProgramBlock, maxLoopSize>();
    ProgramBlock *loopBlock = program.data();

    LoopAnalysis la;

    SECTION("TravellingConstant") {
        loopBlock[0].finalize(MOV, 1, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 1);
        REQUIRE(la.numBootstrapCycles() == 0);

        REQUIRE(la.dataPointerDelta() == 1);
        REQUIRE(la.squashedDataDeltas().size() == 0);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
    }
    SECTION("TravellingConstantWithBootstrap") {
        loopBlock[0].finalize(MOV, 5, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(MOV, -4, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 2);
        REQUIRE(la.numBootstrapCycles() == 4);

        REQUIRE(la.dataPointerDelta() == 1);
        REQUIRE(la.squashedDataDeltas().size() == 0);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);

        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::BOOTSTRAP);
    }
    SECTION("TravellingConstantWithBootstrap2") {
        loopBlock[0].finalize(MOV, 3, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(MOV, -5, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(MOV, 3, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 3);

        REQUIRE(la.numBootstrapCycles() == 3);

        REQUIRE(la.dataPointerDelta() == 1);
        REQUIRE(la.squashedDataDeltas().size() == 0);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);

        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::BOOTSTRAP);

        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::BOOTSTRAP);
    }
    SECTION("TravellingSingleChange") {
        loopBlock[0].finalize(INC, -1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(MOV, 1, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 2);
        REQUIRE(la.numBootstrapCycles() == 0);

        REQUIRE(la.dataPointerDelta() == 1);
        REQUIRE(la.squashedDataDeltas().size() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, 1));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::ANYTIME);

        REQUIRE(la.deltaAt(-1) == 0);
        REQUIRE(la.deltaAt(0) == -1);
        REQUIRE(la.deltaAt(1) == -1);

        REQUIRE(la.deltaAtOnNonStandardEntry(-1, 1) == 0);
        REQUIRE(la.deltaAtOnNonStandardEntry(0, 1) == 0);
        REQUIRE(la.deltaAtOnNonStandardEntry(1, 1) == -1);
    }
    SECTION("TravellingSingleChange2") {
        // As TravellingSingleChange, but with instructions reversed. This, amongst others, changes
        // the number of bootstrap cycles
        loopBlock[0].finalize(MOV, 1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(INC, -1, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 2);
        REQUIRE(la.numBootstrapCycles() == 0);

        REQUIRE(la.dataPointerDelta() == 1);
        REQUIRE(la.squashedDataDeltas().size() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 1));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::ANYTIME);

        REQUIRE(la.deltaAt(-1) == 0);
        REQUIRE(la.deltaAt(0) == 0);
        REQUIRE(la.deltaAt(1) == -1);

        REQUIRE(la.deltaAtOnNonStandardEntry(-1, 1) == 0);
        REQUIRE(la.deltaAtOnNonStandardEntry(0, 1) == -1);
        REQUIRE(la.deltaAtOnNonStandardEntry(1, 1) == -1);
    }
    SECTION("TravellingSingleChange3") {
        // Single change, but with large DP shifts, and modulus DP
        loopBlock[0].finalize(MOV, -5, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(INC, -1, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(MOV,  7, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 3);
        REQUIRE(la.numBootstrapCycles() == 0);

        REQUIRE(la.dataPointerDelta() == 2);
        REQUIRE(la.squashedDataDeltas().size() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 1));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::ANYTIME);

        REQUIRE(la.deltaAt(-7) ==  0);
        REQUIRE(la.deltaAt(-6) ==  0);
        REQUIRE(la.deltaAt(-5) == -1);
        REQUIRE(la.deltaAt(-4) ==  0);
        REQUIRE(la.deltaAt(-3) == -1);
    }
    SECTION("TravellingSingleChange4") {
        // Single change, but with large DP shifts, and modulus DP
        loopBlock[0].finalize(MOV, -4, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(INC, -1, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(MOV,  6, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 3);
        REQUIRE(la.numBootstrapCycles() == 3);

        REQUIRE(la.dataPointerDelta() == 2);
        REQUIRE(la.squashedDataDeltas().size() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 1));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::ANYTIME);

        REQUIRE(la.deltaAt(-6) ==  0);
        REQUIRE(la.deltaAt(-5) ==  0);
        REQUIRE(la.deltaAt(-4) == -1);
        REQUIRE(la.deltaAt(-3) ==  0);
        REQUIRE(la.deltaAt(-2) == -1);
    }
    SECTION("TravellingSingleChangeAndNonZeroExit") {
        loopBlock[0].finalize(MOV, 1, dummySteps, loopBlock + 1, &exitBlock); // SHR   # Expects 0
        loopBlock[1].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 0); // INC

        analyzeLoop(la, loopBlock, 2);

        REQUIRE(la.numBootstrapCycles() == 0);

        REQUIRE(la.dataPointerDelta() == 1);
        REQUIRE(la.squashedDataDeltas().size() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::UNEQUAL, 0));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::NEVER);
    }
    SECTION("TravellingSingleChangeAndNonZeroExit2") {
        // As TravellingSingleChangeAndNonZeroExit, but with order reversed
        loopBlock[0].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 1); // INC
        loopBlock[1].finalize(MOV, 1, dummySteps, loopBlock + 0, &exitBlock); // SHR   # Expects 0

        analyzeLoop(la, loopBlock, 2);
        REQUIRE(la.numBootstrapCycles() == 1);

        REQUIRE(la.dataPointerDelta() == 1);
        REQUIRE(la.squashedDataDeltas().size() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::UNEQUAL, 0));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::ANYTIME);
    }
    SECTION("TravellingSingleChangeInThreeSteps") {
        // A single change realized in three separate increments, each of which can break the loop.
        // Due to complex shifting, the same value is changed in different iterations of the loop.
        // This means that the order of the instructions in the loop does not match the order in
        // which they consume new values. The latter is shown in brackets in the comments below.
        // This order needs to be correctly taken into account to determine which shift
        // instructions have bootstrap-only exits.
        loopBlock[0].finalize(MOV, 2, dummySteps, &exitBlock, loopBlock + 1);  // SHR 2   (3)
        loopBlock[1].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 2);  // INC     (4)
        loopBlock[2].finalize(MOV, 1, dummySteps, &exitBlock, loopBlock + 3);  // SHR     (1)
        loopBlock[3].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 4);  // INC     (2)
        loopBlock[4].finalize(MOV, -2, dummySteps, &exitBlock, loopBlock + 5); // SHL 2   (5)
        loopBlock[5].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 0);  // INC     (6)

        analyzeLoop(la, loopBlock, 6);

        REQUIRE(la.dataPointerDelta() == 1);
        REQUIRE(la.squashedDataDeltas().size() == 1);
        REQUIRE(la.numBootstrapCycles() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, -2));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(3).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(3).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(4).exitCondition.expressionEquals(Operator::EQUALS, -2));
        REQUIRE(la.exit(4).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(5).exitCondition.expressionEquals(Operator::EQUALS, -3));
        REQUIRE(la.exit(5).exitWindow == ExitWindow::ANYTIME);

        REQUIRE(la.deltaAt(0) == 0);
        REQUIRE(la.deltaAt(1) == 1);
        REQUIRE(la.deltaAt(2) == 2);
        REQUIRE(la.deltaAt(3) == 3);

        REQUIRE(la.deltaAtOnNonStandardEntry(-1, 1) == 1);
        REQUIRE(la.deltaAtOnNonStandardEntry( 0, 1) == 2);
        REQUIRE(la.deltaAtOnNonStandardEntry( 1, 1) == 3);
        REQUIRE(la.deltaAtOnNonStandardEntry( 2, 1) == 3);

        REQUIRE(la.deltaAtOnNonStandardEntry(-1, 2) == 1);
        REQUIRE(la.deltaAtOnNonStandardEntry( 0, 2) == 1);
        REQUIRE(la.deltaAtOnNonStandardEntry( 1, 2) == 3);
        REQUIRE(la.deltaAtOnNonStandardEntry( 2, 2) == 3);

        REQUIRE(la.deltaAtOnNonStandardEntry(-1, 4) == 1);
        REQUIRE(la.deltaAtOnNonStandardEntry( 0, 4) == 2);
        REQUIRE(la.deltaAtOnNonStandardEntry( 1, 4) == 3);

        REQUIRE(la.deltaAtOnNonStandardEntry(-1, 5) == 0);
        REQUIRE(la.deltaAtOnNonStandardEntry( 0, 5) == 1);
        REQUIRE(la.deltaAtOnNonStandardEntry( 1, 5) == 1);
    }
    SECTION("TravellingSingleChangeInThreeSteps2") {
        // As TravellingSingleChangeInThreeSteps, but with shifts reversed. This should not impact
        // the exit analysis, which this test intends to verify.
        loopBlock[0].finalize(MOV, -2, dummySteps, &exitBlock, loopBlock + 1); // SHL 2   (3)
        loopBlock[1].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 2);  // INC     (4)
        loopBlock[2].finalize(MOV, -1, dummySteps, &exitBlock, loopBlock + 3); // SHL     (1)
        loopBlock[3].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 4);  // INC     (2)
        loopBlock[4].finalize(MOV, 2, dummySteps, &exitBlock, loopBlock + 5);  // SHR 2   (5)
        loopBlock[5].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 0);  // INC     (6)

        analyzeLoop(la, loopBlock, 6);

        REQUIRE(la.dataPointerDelta() == -1);
        REQUIRE(la.squashedDataDeltas().size() == 1);
        REQUIRE(la.numBootstrapCycles() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, -2));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(3).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(3).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(4).exitCondition.expressionEquals(Operator::EQUALS, -2));
        REQUIRE(la.exit(4).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(5).exitCondition.expressionEquals(Operator::EQUALS, -3));
        REQUIRE(la.exit(5).exitWindow == ExitWindow::ANYTIME);
    }
    SECTION("TravellingSingleChangeInThreeSteps3") {
        // Similar to TravellingSingleChangeInThreeSteps but with a single shift change, which
        // changes the order in which instructions consume new values and leads to different
        // bootstrap-only exits. Also, it increases the number of bootstrap cycles.
        loopBlock[0].finalize(MOV, 2, dummySteps, &exitBlock, loopBlock + 1);  // SHR 2   (3)
        loopBlock[1].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 2);  // INC     (4)
        loopBlock[2].finalize(MOV, 1, dummySteps, &exitBlock, loopBlock + 3);  // SHR     (5)
        loopBlock[3].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 4);  // INC     (6)
        loopBlock[4].finalize(MOV, -4, dummySteps, &exitBlock, loopBlock + 5); // SHL 4   (1)
        loopBlock[5].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 0);  // INC     (2)

        analyzeLoop(la, loopBlock, 6);

        REQUIRE(la.dataPointerDelta() == -1);
        REQUIRE(la.squashedDataDeltas().size() == 1);
        REQUIRE(la.numBootstrapCycles() == 3);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, -2));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::EQUALS, -2));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(3).exitCondition.expressionEquals(Operator::EQUALS, -3));
        REQUIRE(la.exit(3).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(4).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(4).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(5).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(5).exitWindow == ExitWindow::ANYTIME);
    }
    SECTION("TravellingTwoChanges") {
        loopBlock[0].finalize(MOV, 1, dummySteps, &exitBlock, loopBlock + 1);  // SHR
        loopBlock[1].finalize(INC, -1, dummySteps, &exitBlock, loopBlock + 2); // DEC
        loopBlock[2].finalize(MOV, 1, dummySteps, &exitBlock, loopBlock + 3);  // SHR
        loopBlock[3].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 0);  // INC

        analyzeLoop(la, loopBlock, 4);

        REQUIRE(la.dataPointerDelta() == 2);
        REQUIRE(la.minDp() == 0);
        REQUIRE(la.maxDp() == 2);
        REQUIRE(la.squashedDataDeltas().size() == 2);
        REQUIRE(la.numBootstrapCycles() == 0);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 1));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(3).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(3).exitWindow == ExitWindow::ANYTIME);
    }
    SECTION("TravellingTwoChanges2") {
        // A variant of TravellingOscillating2 but with modified shifts so that deltas do not
        // cancel each other out
        loopBlock[0].finalize(MOV, -1, dummySteps, &exitBlock, loopBlock + 1); // SHL
        loopBlock[1].finalize(INC, -1, dummySteps, &exitBlock, loopBlock + 2); // DEC
        loopBlock[2].finalize(MOV, 2, dummySteps, &exitBlock, loopBlock + 3);  // SHR 2
        loopBlock[3].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 4);  // INC
        loopBlock[4].finalize(MOV, 2, dummySteps, &exitBlock, loopBlock + 0);  // SHR 2

        analyzeLoop(la, loopBlock, 5);

        REQUIRE(la.dataPointerDelta() == 3);
        REQUIRE(la.minDp() == -1);
        REQUIRE(la.maxDp() == 3);
        REQUIRE(la.squashedDataDeltas().size() == 2);
        REQUIRE(la.numBootstrapCycles() == 0);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 1));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(3).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(3).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(4).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(4).exitWindow == ExitWindow::ANYTIME);
    }
    SECTION("TravellingTwoChanges3") {
        // A variant of TravellingOscillating2 but with modified shifts so that deltas do not
        // cancel each other out
        loopBlock[0].finalize(INC,  1, dummySteps, &exitBlock, loopBlock + 1);
        loopBlock[1].finalize(MOV,  3, dummySteps, &exitBlock, loopBlock + 2);
        loopBlock[2].finalize(INC,  2, dummySteps, &exitBlock, loopBlock + 3);
        loopBlock[3].finalize(MOV, -2, dummySteps, &exitBlock, loopBlock + 0);

        analyzeLoop(la, loopBlock, 4);
        REQUIRE(la.dataPointerDelta() == 1);
        REQUIRE(la.minDp() == 0);
        REQUIRE(la.maxDp() == 3);
        REQUIRE(la.squashedDataDeltas().size() == 1);
        REQUIRE(la.squashedDataDeltas().deltaAt(0) == 3);
    }
    SECTION("TravellingOscillating") {
        loopBlock[0].finalize(MOV, 2, dummySteps, &exitBlock, loopBlock + 1);  // SHR 2
        loopBlock[1].finalize(INC, -1, dummySteps, &exitBlock, loopBlock + 2); // DEC
        loopBlock[2].finalize(MOV, -1, dummySteps, &exitBlock, loopBlock + 3); // SHL
        loopBlock[3].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 0);  // INC

        analyzeLoop(la, loopBlock, 4);

        REQUIRE(la.dataPointerDelta() == 1);
        REQUIRE(la.squashedDataDeltas().size() == 0);
        REQUIRE(la.numBootstrapCycles() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 1));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::EQUALS, 1));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(3).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(3).exitWindow == ExitWindow::BOOTSTRAP);
    }
    SECTION("TravellingOscillating2") {
        // A more complex oscillating change, where original DP-offsets are both positive and
        // negative, which requires sign-aware modulus matching to collapse these entries.
        loopBlock[0].finalize(MOV, -1, dummySteps, &exitBlock, loopBlock + 1); // SHL
        loopBlock[1].finalize(INC, -1, dummySteps, &exitBlock, loopBlock + 2); // DEC
        loopBlock[2].finalize(MOV, 3, dummySteps, &exitBlock, loopBlock + 3);  // SHR 3
        loopBlock[3].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 4);  // INC
        loopBlock[4].finalize(MOV, 1, dummySteps, &exitBlock, loopBlock + 0);  // SHR

        analyzeLoop(la, loopBlock, 5);

        REQUIRE(la.dataPointerDelta() == 3);
        REQUIRE(la.squashedDataDeltas().size() == 0);
        REQUIRE(la.numBootstrapCycles() == 1);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(3).exitCondition.expressionEquals(Operator::EQUALS, -1));
        REQUIRE(la.exit(3).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(4).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(4).exitWindow == ExitWindow::ANYTIME);
    }
    SECTION("TravellingOscillatingWithNonZeroExit") {
        // A loop that occurs in BB 7x7 #950175 which was wrongly analysed.
        //
        // Note: This is a short-running loop. It cannot complete three iterations, as it has two
        // conficting exit conditions (Intruction 1 and 2). It can, however, complete its two
        // bootstrap iterations, which is enough to classify it as a loop.
        loopBlock[0].finalize(MOV, -1, dummySteps, &exitBlock, loopBlock + 1); // SHL
        loopBlock[1].finalize(INC, 2, dummySteps, &exitBlock, loopBlock + 2);  // INC 2
        loopBlock[2].finalize(MOV, 2, dummySteps, loopBlock + 3, &exitBlock);  // SHR 2 # Expects 0
        loopBlock[3].finalize(INC, 1, dummySteps, &exitBlock, loopBlock + 4);  // INC
        loopBlock[4].finalize(INC, -3, dummySteps, &exitBlock, loopBlock + 0); // DEC 3

        analyzeLoop(la, loopBlock, 5);

        REQUIRE(la.dataPointerDelta() == 1);
        REQUIRE(la.squashedDataDeltas().size() == 0);
        REQUIRE(la.numBootstrapCycles() == 2);

        REQUIRE(la.exit(0).exitCondition.expressionEquals(Operator::EQUALS, 2));
        REQUIRE(la.exit(0).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(1).exitCondition.expressionEquals(Operator::EQUALS, 0));
        REQUIRE(la.exit(1).exitWindow == ExitWindow::BOOTSTRAP);
        REQUIRE(la.exit(2).exitCondition.expressionEquals(Operator::UNEQUAL, 0));
        REQUIRE(la.exit(2).exitWindow == ExitWindow::ANYTIME);
        REQUIRE(la.exit(3).exitWindow == ExitWindow::NEVER);
        REQUIRE(la.exit(4).exitWindow == ExitWindow::NEVER);
    }
}
