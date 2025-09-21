//
//  FastExecutorTests.cpp
//  Tests
//
//  Created by Erwin on 15/08/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include "catch.hpp"

#include "FastExecutor.h"
#include "InterpretedProgramBuilder.h"
#include "Program.h"

TEST_CASE("6x6 Fast Executor tests", "[6x6][fast-exec]") {
    FastExecutor fastExecutor(1024);

    SECTION("6x6-FastExecution-DataError") {
        // Program that results in a DATA_ERROR. It erroneously resulted in a PROGRAM_ERROR in an
        // earlier version.
        //
        //   * *   *
        // * _ o _ o *
        // o _ _ * *
        // _ * _ _ _
        // _ * o _ _ *
        // _   *
        Program program = Program::fromString("ZiiIRkKCACQggA");
        auto programBuilder = std::make_shared<InterpretedProgramBuilder>();
        programBuilder->buildFromProgram(program);

        // Act
        RunResult result = fastExecutor.execute(programBuilder);

        REQUIRE(result == RunResult::DATA_ERROR);
    }
}
