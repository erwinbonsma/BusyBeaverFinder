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
#include "SearchOrchestrator.h"

ExhaustiveSearcher* searcher;
ProgressTracker* tracker;

Ins* resumeStack = nullptr;

void init(int argc, char * argv[]) {
    cxxopts::Options options("BusyBeaverFinder", "Searcher for Busy Beaver Programs");
    options.add_options()
        ("w,width", "Program width", cxxopts::value<int>())
        ("h,height", "Program height", cxxopts::value<int>())
        ("d,datasize", "Data size", cxxopts::value<int>())
        ("max-steps", "Maximum program execution steps", cxxopts::value<int>())
        ("p,hang-period", "Initial hang detection period", cxxopts::value<int>())
        ("periodic-attempts", "Maximum Periodic Hang detect attempts", cxxopts::value<int>())
        ("regsweep-attempts", "Maximum Regular Sweep Hang detect attempts", cxxopts::value<int>())
        ("max-regular-sweeps", "Maximum Number of Sweeps", cxxopts::value<int>())
        ("resume-from", "File with resume stack", cxxopts::value<std::string>())
        ("t,test-hangs", "Test hang detection")
        ("dump-period", "The period of dumping basic stats", cxxopts::value<int>())
        ("dump-undetected-hangs", "Report undetected hangs")
        ("help", "Show help");
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help({"", "Group"}) << std::endl;
        exit(0);
    }

    int width = 4;
    int height = 4;
    int dataSize = 1024;

    if (result.count("w")) {
        width = result["w"].as<int>();
    }
    if (result.count("h")) {
        height = result["h"].as<int>();
    }
    if (result.count("d")) {
        dataSize = result["d"].as<int>();
    }

    searcher = new ExhaustiveSearcher(width, height, dataSize);

    SearchSettings settings = searcher->getSettings();

    // Set hang sample period
    if (result.count("p")) {
        settings.initialHangSamplePeriod = result["p"].as<int>();
    }
    if (! isPowerOfTwo(settings.initialHangSamplePeriod) ) {
        settings.initialHangSamplePeriod = makePowerOfTwo(settings.initialHangSamplePeriod);
        std::cout << "Adjusted initialHangSamplePeriod to be a power of two" << std::endl;
    }

    // Set max total steps
    if (result.count("max-steps")) {
        settings.maxSteps = result["max-steps"].as<int>();
    }

    if (result.count("periodic-attempts")) {
        settings.maxPeriodicHangDetectAttempts = result["periodic-attempts"].as<int>();
    }
    if (result.count("regsweep-attempts")) {
        settings.maxRegularSweepHangDetectAttempts = result["regsweep-attempts"].as<int>();
    }
    if (result.count("max-regular-sweeps")) {
        settings.maxRegularSweepExtensionCount = result["max-regular-sweeps"].as<int>();
    }

    // Enable testing of hang detection?
    if (result.count("t")) {
        settings.testHangDetection = true;
    }

    searcher->configure(settings);

    // Load resume stack
    if (result.count("resume-from")) {
        std::string resumeFile = result["resume-from"].as<std::string>();

        resumeStack = loadResumeStackFromFile(resumeFile, width * height);
    }

    tracker = new ProgressTracker(*searcher);
    if (result.count("dump-period")) {
        tracker->setDumpStatsPeriod(result["dump-period"].as<int>());
    }
    if (result.count("dump-undetected-hangs")) {
        tracker->setDumpUndetectedHangs(true);
    }

    searcher->setProgressTracker(tracker);
}

int main(int argc, char * argv[]) {
    init(argc, argv);

    searcher->dumpSettings();
    if (resumeStack != nullptr) {
        searcher->search((Ins*)resumeStack);
    } else {
        SearchOrchestrator orchestrator(*searcher);
        orchestrator.search();
    }
    tracker->dumpFinalStats();

    return 0;
}
