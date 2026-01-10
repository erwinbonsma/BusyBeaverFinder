#include "catch.hpp"

#include <time.h>
#include <memory>

#include "FastExecutor.h"
#include "HangExecutor.h"
#include "InterpretedProgramBuilder.h"
#include "Program.h"

TEST_CASE("Executor performance tests", "[perf][.explicit]") {
    Program program = Program::fromString("Zv6+kpUoAqW0bw");
    auto programBuilder = std::make_shared<InterpretedProgramBuilder>();
    programBuilder->buildFromProgram(program);

    std::unique_ptr<ProgramExecutor> executor;

    int dataSize = 1000000;
    int maxSteps = 20000000;
    std::string name;

    SECTION("perf-FastExecutor") {
        name = "fast";
        executor = std::make_unique<FastExecutor>(dataSize);
    }
    SECTION("perf-HangExecutor-UndoOnly") {
        name = "undo";
        executor = std::make_unique<HangExecutor>(dataSize, 0);
    }
    SECTION("perf-HangExecutor") {
        name = "hang+undo";
        executor = std::make_unique<HangExecutor>(dataSize, maxSteps);
    }

    executor->setMaxSteps(maxSteps);

    clock_t startTime = clock();
    RunResult result = executor->execute(programBuilder);
    REQUIRE(result == RunResult::ASSUMED_HANG);

    std::cout << "Executor " << name << ": "
    << (clock() - startTime) / (double)CLOCKS_PER_SEC << std::endl;
}
