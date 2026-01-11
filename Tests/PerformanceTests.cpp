#include "catch.hpp"

#include <time.h>
#include <memory>

#include "FastExecutor.h"
#include "HangExecutor.h"
#include "InterpretedProgramBuilder.h"
#include "Program.h"

TEST_CASE("Executor performance tests", "[perf][.explicit]") {
    std::string programSpec{"Zv6+kpUoAqW0bw"};
    Program program = Program::fromString(programSpec);
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

TEST_CASE("Program construction performance", "[perf][.explicit][program]") {
    std::string spec = "Zv6+kpUoAqW0bw";

    clock_t startTime = clock();
    std::string label;

    SECTION("perf-program-construction") {
        label = "construction";
        for (int i = 0; i < 1000000; i++) {
            Program program = Program::fromString(spec);

            REQUIRE(spec == program.toString());
        }
    }
    SECTION("pref-program-copy assignment") {
        label = "copy-assign";
        Program program2;
        for (int i = 0; i < 1000000; i++) {
            Program program = Program::fromString(spec);
            program2 = program;

            REQUIRE(spec == program2.toString());
        }
    }
    SECTION("pref-program-move assignment") {
        label = "move-assign";
        Program program2;
        for (int i = 0; i < 1000000; i++) {
            Program program = Program::fromString(spec);
            program2 = std::move(program);

            REQUIRE(spec == program2.toString());
        }
    }

    std::cout << "Program " << label << ": "
    << (clock() - startTime) / (double)CLOCKS_PER_SEC << std::endl;
}
