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

ExhaustiveSearcher* searcher;
ProgressTracker* tracker;

Op* resumeStack = nullptr;

void init(int argc, char * argv[]) {
    cxxopts::Options options("BusyBeaverFinder", "Searcher for Busy Beaver Programs");
    options.add_options()
        ("w,width", "Program width", cxxopts::value<int>())
        ("h,height", "Program height", cxxopts::value<int>())
        ("d,datasize", "Data size", cxxopts::value<int>())
        ("max-steps", "Maximum steps per recursion level", cxxopts::value<int>())
        ("max-steps-total", "Total maximum steps", cxxopts::value<int>())
        ("p,hang-period", "Period for hang-detection", cxxopts::value<int>())
        ("resume-from", "File with resume stack", cxxopts::value<std::string>())
        ("t,test-hangs", "Test hang detection")
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

    // Set hang sample period
    int hangSamplePeriod = searcher->getHangSamplePeriod();
    if (result.count("p")) {
        hangSamplePeriod = result["p"].as<int>();
    }
    if (! isPowerOfTwo(hangSamplePeriod) ) {
        hangSamplePeriod = makePowerOfTwo(hangSamplePeriod);
        std::cout << "Adjusted hangSamplePeriod to be a power of two" << std::endl;
    }
    searcher->setHangSamplePeriod(hangSamplePeriod);

    // Set max steps per run
    int maxStepsPerRun = searcher->getMaxStepsPerRun();
    if (result.count("max-steps")) {
        maxStepsPerRun = result["max-steps"].as<int>();
    }
    if (maxStepsPerRun % hangSamplePeriod != 0) {
        maxStepsPerRun += hangSamplePeriod - (maxStepsPerRun % hangSamplePeriod);
        std::cout << "Adjusted maxStepsPerRun to be multiple of hangSamplePeriod" << std::endl;
    }
    searcher->setMaxStepsPerRun( maxStepsPerRun );

    // Set max total steps
    int maxStepsTotal = searcher->getMaxStepsTotal();
    if (result.count("max-steps-total")) {
        maxStepsTotal = result["max-steps-total"].as<int>();
    }
    if (maxStepsTotal < maxStepsPerRun) {
        maxStepsTotal = maxStepsPerRun;
        std::cout << "Adjusted maxStepsTotal to equal maxStepsPerRun" << std::endl;
    }
    searcher->setMaxStepsTotal( maxStepsTotal );

    // Enable testing of hang detection?
    if (result.count("t")) {
        searcher->setHangDetectionTestMode(true);
    }

    // Load resume stack
    if (result.count("resume-from")) {
        std::string resumeFile = result["resume-from"].as<std::string>();

        resumeStack = loadResumeStackFromFile(resumeFile, width * height);
    }

    tracker = new ProgressTracker(searcher);
    searcher->setProgressTracker(tracker);
}

int main(int argc, char * argv[]) {
    init(argc, argv);

    searcher->dumpSettings();
    if (resumeStack != nullptr) {
        searcher->search((Op*)resumeStack);
    } else {
        searcher->search();
    }
    tracker->dumpFinalStats();

    return 0;
}
