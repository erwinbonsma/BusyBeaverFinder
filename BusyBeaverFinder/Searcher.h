//
//  Searcher.h
//  BusyBeaverFinder
//
//  Created by Erwin on 11/01/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//
#pragma once

#include "Program.h"
#include "ProgressTracker.h"

struct BaseSearchSettings {
    ProgramSize size{6};

    int dataSize{1024};

    // The maximum steps that a program will run for.
    int maxSteps{1024};

    BaseSearchSettings(int size) : size(size) {}
};

class Searcher {
protected:
    Program _program;
    std::unique_ptr<ProgressTracker> _tracker;

public:
    Searcher(ProgramSize size);

    void attachProgressTracker(std::unique_ptr<ProgressTracker> tracker);
    std::unique_ptr<ProgressTracker> detachProgressTracker();

    virtual int getNumSteps() const = 0;
    const Program& getProgram() const { return _program; }

    virtual void dumpSettings(std::ostream &os) const = 0;
    virtual void dumpSearchProgress(std::ostream &os) const = 0;
};
