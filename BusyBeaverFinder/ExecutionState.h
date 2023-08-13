//
//  ExecutionState.h
//  BusyBeaverFinder
//
//  Created by Erwin on 25/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//
#pragma once

class Data;
class InterpretedProgram;
class RunSummary;

/* Abstract data type. Interface to hang detectors
 */
class ExecutionState {

public:
    virtual const InterpretedProgram* getInterpretedProgram() const = 0;

    virtual const Data& getData() const = 0;

    virtual const RunSummary& getRunSummary() const = 0;
    virtual const RunSummary& getMetaRunSummary() const = 0;

    virtual void dumpExecutionState() const;
};
