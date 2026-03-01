//
//  main.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <iostream>

#include "cxxopts.hpp"

#include "Utils.h"
#include "ExhaustiveSearcher.h"
#include "ProgressTracker.h"
#include "SearchOrchestration.h"

enum class RunMode : int8_t {
    // Perform a full (orchestrated) search
    FULL_SEARCH = 0,

    // Resume a full search from a given search state (instruction stack)
    //
    // Note: This does _not_ perform an orchestrated search, so is not suitable for carrying out
    // a full search. It is mainly useful to reproduce an issue encountered with a previous search.
    RESUME_FROM = 1,

    // Perform a sub-tree search for each program in a list of "late escapees"
    LATE_ESCAPE = 2,

    // No search. Just execute each of the programs to see how they behave.
    ONLY_RUN = 3,
};

std::shared_ptr<SearchRunner> searchRunner;

void init(int argc, char * argv[]) {
    cxxopts::Options options("BusyBeaverFinder", "Searcher for Busy Beaver Programs");
    options.add_options()
        ("w,width", "Program width", cxxopts::value<int>())
        ("h,height", "Program height", cxxopts::value<int>())
        ("d,datasize", "Data size", cxxopts::value<int>())
        ("max-steps", "Maximum program execution steps", cxxopts::value<int>())
        ("max-search-steps", "Maximum steps to enable back-tracking search",
         cxxopts::value<int>())
        ("max-hang-detection-steps", "Max steps to execute with hang detection",
         cxxopts::value<int>())
        ("undo-capacity", "Maximum data operations to undo", cxxopts::value<int>())
        ("run-mode", "One of: FULL, RESUME, ESCAPE, ONLYRUN", cxxopts::value<std::string>())
        ("input-file", "File with programs (ESCAPE, ONLYRUN)", cxxopts::value<std::string>())
        ("resume-from", "Program from which to resume the search", cxxopts::value<std::string>())
        ("t,test-hangs", "Test hang detection")
        ("dump-period", "The period of dumping basic stats", cxxopts::value<int>())
        ("dump-success-steps-limit", "The minimum number of steps for dumping successful programs",
         cxxopts::value<int>())
        ("dump-undetected-hangs", "Report undetected hangs")
        ("help", "Show help");
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help({"", "Group"}) << std::endl;
        exit(0);
    }

    SearchSettings settings{6};

    if (result.count("w")) {
        settings.size.width = result["w"].as<int>();
    }
    if (result.count("h")) {
        settings.size.height = result["h"].as<int>();
    }

    RunMode runMode = result.count("resume-from") ? RunMode::RESUME_FROM : RunMode::FULL_SEARCH;
    if (result.count("run-mode")) {
        auto s = result["run-mode"].as<std::string>();
        if (s == "FULL") {
            runMode = RunMode::FULL_SEARCH;
        } else if (s == "RESUME") {
            runMode = RunMode::RESUME_FROM;
        } else if (s == "ESCAPE") {
            runMode = RunMode::LATE_ESCAPE;
        } else if (s == "ONLYRUN") {
            runMode = RunMode::ONLY_RUN;
        } else {
            std::cerr << "Unknown run mode: " << s << std::endl;
            exit(-1);
        }
    }

    if (result.count("d")) {
        settings.dataSize = result["d"].as<int>();
    }

    // Set max total steps
    if (result.count("max-steps")) {
        settings.maxSteps = result["max-steps"].as<int>();
    }
    if (result.count("max-hang-detection-steps")) {
        settings.maxHangDetectionSteps = result["max-hang-detection-steps"].as<int>();
    }
    if (result.count("max-search-steps")) {
        settings.maxSearchSteps = result["max-search-steps"].as<int>();
    }
    settings.maxSearchSteps = std::max(settings.maxHangDetectionSteps, settings.maxSearchSteps);
    settings.maxSteps = std::max(settings.maxSearchSteps, settings.maxSteps);

    // Enable testing of hang detection?
    if (result.count("t")) {
        settings.testHangDetection = true;
    }

    std::string inputFile;
    if (result.count("input-file")) {
        inputFile = result["input-file"].as<std::string>();
    }
    bool expectsInputFile = runMode == RunMode::ONLY_RUN || runMode == RunMode::LATE_ESCAPE;
    if (inputFile.empty()) {
        if (expectsInputFile) {
            std::cerr << "Missing input file" << std::endl;
            exit(-1);
        }
    } else {
        if (!expectsInputFile) {
            std::cout << "Ignoring input file" << std::endl;
        }
    }

    switch (runMode) {
        case RunMode::ONLY_RUN:
            searchRunner = std::make_shared<FastExecSearchRunner>(settings, inputFile);
            break;
        case RunMode::FULL_SEARCH:
            searchRunner = std::make_shared<OrchestratedSearchRunner>(settings);
            break;
        case RunMode::RESUME_FROM: {
            std::string resumeFrom = result["resume-from"].as<std::string>();
            searchRunner = std::make_shared<ResumeSearchRunner>(settings, resumeFrom);
            break;
        }
        case RunMode::LATE_ESCAPE:
            searchRunner = std::make_shared<LateEscapeSearchRunner>(settings, inputFile);
            break;
    }
    searchRunner->getSearcher().dumpSettings(std::cout);

    auto tracker = std::make_unique<ProgressTracker>();

    if (result.count("dump-period")) {
        tracker->setDumpStatsPeriod(result["dump-period"].as<int>());
    }
    if (result.count("dump-undetected-hangs")) {
        tracker->setDumpUndetectedHangs(true);
    }
    if (result.count("dump-success-steps-limit")) {
        tracker->setDumpSuccessStepsLimit(result["dump-success-steps-limit"].as<int>());
    }

    if (runMode == RunMode::ONLY_RUN || runMode == RunMode::LATE_ESCAPE) {
        tracker->setDumpSuccessStepsLimit(0); // Dump every successful program
    }

    searchRunner->getSearcher().attachProgressTracker(std::move(tracker));
}

int main(int argc, char * argv[]) {
    init(argc, argv);

    searchRunner->run();

    auto tracker = searchRunner->detachProgressTracker();
    tracker->dumpFinalStats();

    return 0;
}
