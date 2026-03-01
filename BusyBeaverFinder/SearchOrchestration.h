//
//  SearchOrchestrator.h
//  BusyBeaverFinder
//
//  Created by Erwin on 11/02/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include <string>
#include <vector>

#include "Types.h"
#include "Searcher.h"
#include "ExhaustiveSearcher.h"
#include "FastExecSearcher.h"

class ExhaustiveSearcher;

class SearchRunner {
public:
    virtual void run() = 0;

    virtual Searcher& getSearcher() = 0;
    std::unique_ptr<ProgressTracker> detachProgressTracker() {
        return getSearcher().detachProgressTracker();
    }
};

class OrchestratedSearchRunner : public SearchRunner {
    ExhaustiveSearcher _searcher;

    void addInstructionsUntilTurn(std::vector<Ins> &stack, int numNoop, int numData);
public:
    OrchestratedSearchRunner(SearchSettings settings) : _searcher(settings) {}

    ExhaustiveSearcher& getSearcher() override { return _searcher; };
    void run() override;
};

class ResumeSearchRunner : public SearchRunner {
    ExhaustiveSearcher _searcher;
    std::string _programSpec;
public:
    ResumeSearchRunner(SearchSettings settings, std::string programSpec)
    : _searcher(settings), _programSpec(programSpec) {}

    ExhaustiveSearcher& getSearcher() override { return _searcher; };
    void run() override;
};

class LateEscapeSearchRunner : public SearchRunner {
    ExhaustiveSearcher _searcher;
    std::string _programFile;
public:
    LateEscapeSearchRunner(SearchSettings settings, std::string programFile)
    : _searcher(settings), _programFile(programFile) {}

    ExhaustiveSearcher& getSearcher() override { return _searcher; };
    void run() override;
};

class FastExecSearchRunner : public SearchRunner {
    FastExecSearcher _searcher;
    std::string _programFile;
public:
    FastExecSearchRunner(BaseSearchSettings settings, std::string programFile)
    : _searcher(settings), _programFile(programFile) {}

    FastExecSearcher& getSearcher() override { return _searcher; };
    void run() override;
};
