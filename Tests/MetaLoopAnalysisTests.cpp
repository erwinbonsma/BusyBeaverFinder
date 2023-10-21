//
//  MetaLoopAnalysisTests.cpp
//  Tests
//
//  Created by Erwin on 18/10/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "HangDetector.h"
#include "ProgramBlock.h"
#include "MetaLoopAnalysis.h"

const int dummySteps = 1;
const int maxSequenceLen = 10;

const int INC = true;
const int MOV = false;

class RunUntilMetaLoop : public HangDetector {
    int _numIterations;

protected:
    bool shouldCheckNow(bool loopContinues) const override {
        if (!loopContinues) {
            _execution.dumpExecutionState();
        }

        return (!loopContinues
                && _execution.getMetaRunSummary().isInsideLoop()
                && _execution.getMetaRunSummary().getLoopIteration() >= _numIterations);
    }

    bool analyzeHangBehaviour() override { return true; };
    Trilian proofHang() override { return Trilian::YES; };

public:
    RunUntilMetaLoop(const ExecutionState& execution, int numIterations = 3)
    : HangDetector(execution), _numIterations(numIterations) {}
};

TEST_CASE( "Meta-loop", "[meta-loop-analysis]" ) {
    HangExecutor hangExecutor(1000, 1000);
    hangExecutor.setMaxSteps(1000);
    hangExecutor.addHangDetector(std::make_shared<RunUntilMetaLoop>(hangExecutor));

    ProgramBlock block[maxSequenceLen];
    for (int i = 0; i < maxSequenceLen; i++) {
        block[i].init(i);
    }
    ProgramBlock *exitBlock = &block[maxSequenceLen - 1];

    MetaLoopAnalysis mla;

    SECTION( "Program-BasicRightSweep" ) {
        // Sweep body consists of only ones and extends by one position to the right
        block[0].finalize(MOV,  1, dummySteps, block + 1, block + 0);
        block[1].finalize(INC,  1, dummySteps, exitBlock, block + 2);
        block[2].finalize(MOV, -1, dummySteps, block + 0, block + 2);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        program.dump();

        hangExecutor.execute(&program);
    }

    SECTION( "Program-BasicLeftSweep" ) {
        // Sweep body consists of only ones and extends by one position to the left
        block[0].finalize(MOV,  1, dummySteps, block + 1, block + 0);
        block[1].finalize(MOV, -1, dummySteps, block + 2, block + 1);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 0);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        program.dump();

        hangExecutor.execute(&program);
    }

    SECTION( "Program-BasicDualEndedSweep" ) {
        // Sweep body consists of only ones (at the right) and minus ones (at the left) and extends
        // by one position to the left and right
        block[0].finalize(MOV,  1, dummySteps, block + 1, block + 0);
        block[1].finalize(INC,  1, dummySteps, exitBlock, block + 2);
        block[2].finalize(MOV, -1, dummySteps, block + 3, block + 2);
        block[3].finalize(INC, -1, dummySteps, exitBlock, block + 0);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        program.dump();

        hangExecutor.execute(&program);
    }

    SECTION( "Program-SimpleGlider" ) {
        // Glider counter increases by one each iteration
        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 2, block + 2);
        block[2].finalize(INC,  1, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, exitBlock, block + 4);
        block[4].finalize(INC, -1, dummySteps, block + 5, block + 1);
        block[5].finalize(MOV,  2, dummySteps, block + 6, exitBlock);
        block[6].finalize(INC,  1, dummySteps, exitBlock, block + 2);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        program.dump();

        hangExecutor.execute(&program);
    }

    SECTION( "Program-PowersOfTwo" ) {
        // Calculates powers of two using two stationary counters that alternate between one being
        // decremented and the other being incremented

        block[0].finalize(INC,  1, dummySteps, exitBlock, block + 1);
        block[1].finalize(MOV,  1, dummySteps, block + 2, block + 2);
        block[2].finalize(INC,  2, dummySteps, exitBlock, block + 3);
        block[3].finalize(MOV, -1, dummySteps, exitBlock, block + 4);
        block[4].finalize(INC, -1, dummySteps, block + 6, block + 1);
        block[5].finalize(MOV, -1, dummySteps, exitBlock, block + 6);
        block[6].finalize(INC,  1, dummySteps, exitBlock, block + 7);
        block[7].finalize(MOV,  1, dummySteps, exitBlock, block + 8);
        block[8].finalize(INC, -1, dummySteps, block + 2, block + 5);

        InterpretedProgramFromArray program(block, maxSequenceLen);
        program.dump();

        hangExecutor.execute(&program);
    }
}
