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

ExhaustiveSearcher* searcher;
ProgressTracker* tracker;

std::vector<Ins> resumeStack;
std::string lateEscapeFile;

void init(int argc, char * argv[]) {
    cxxopts::Options options("BusyBeaverFinder", "Searcher for Busy Beaver Programs");
    options.add_options()
        ("w,width", "Program width", cxxopts::value<int>())
        ("h,height", "Program height", cxxopts::value<int>())
        ("d,datasize", "Data size", cxxopts::value<int>())
        ("max-steps", "Maximum program execution steps", cxxopts::value<int>())
        ("max-hang-detection-steps", "Max steps to execute with hang detection", cxxopts::value<int>())
        ("undo-capacity", "Maximum data operations to undo", cxxopts::value<int>())
        ("resume-from", "File with resume stack", cxxopts::value<std::string>())
        ("late-escapes", "File with late escapes", cxxopts::value<std::string>())
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

    if (result.count("w")) {
        width = result["w"].as<int>();
    }
    if (result.count("h")) {
        height = result["h"].as<int>();
    }

    SearchSettings settings = defaultSearchSettings;

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
    if (settings.maxSteps < settings.maxHangDetectionSteps) {
        settings.maxSteps = settings.maxHangDetectionSteps;
    }

    // Enable testing of hang detection?
    if (result.count("t")) {
        settings.testHangDetection = true;
    }

    searcher = new ExhaustiveSearcher(width, height, settings);

    // Load resume stack
    if (result.count("resume-from")) {
        std::string resumeFile = result["resume-from"].as<std::string>();

        if (!loadResumeStackFromFile(resumeFile, resumeStack)) {
            std::cerr << "Failed to read resume stack from " << resumeFile << std::endl;
        }
    }

    tracker = new ProgressTracker(*searcher);

    if (result.count("late-escapes")) {
        lateEscapeFile = result["late-escapes"].as<std::string>();
        tracker->setDumpDone(true);
    }

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
    if (!resumeStack.empty()) {
        searcher->search(resumeStack);
    }
    else if (!lateEscapeFile.empty()) {
        searchLateEscapes(*searcher, lateEscapeFile);
    }
    else {
        orchestratedSearch(*searcher);
    }
    tracker->dumpFinalStats();

    return 0;
}
