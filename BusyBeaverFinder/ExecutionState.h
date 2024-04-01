//
//  ExecutionState.h
//  BusyBeaverFinder
//
//  Created by Erwin on 25/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//
#pragma once

#include "RunSummary.h"

class Data;
class InterpretedProgram;
class RunBlockTransitions;

/* Abstract data type. Interface to hang detectors
 */
class ExecutionState {

public:
    virtual const InterpretedProgram* getInterpretedProgram() const = 0;

    virtual const Data& getData() const = 0;

    virtual const RunHistory& getRunHistory() const = 0;
    virtual const RunSummary& getRunSummary() const = 0;
    virtual const MetaRunSummary& getMetaRunSummary() const = 0;
    virtual const RunBlockTransitions& getRunBlockTransitions() const = 0;

    virtual void dumpExecutionState() const;
};
