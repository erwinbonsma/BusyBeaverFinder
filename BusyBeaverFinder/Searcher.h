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

    virtual void dumpSearchProgress(std::ostream &os) const = 0;
};
