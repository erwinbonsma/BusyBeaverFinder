//
//  ProgramExecutor.h
//  BusyBeaverFinder
//
//  Created by Erwin on 12/08/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

class ProgramBlock;

enum class RunResult {
    // Program completed successfully
    SUCCESS = 0,

    // Error execution data instruction
    DATA_ERROR = 1,

    // Encountered an unfinalized program block
    PROGRAM_ERROR = 2,

    // A hang detector detected a hang
    DETECTED_HANG = 3,

    // The program did not complete within the limit allowed steps
    ASSUMED_HANG = 4,
};

class ProgramExecutor {

public:
    virtual RunResult execute(const ProgramBlock *programBlock, int maxSteps) = 0;
    virtual int numSteps() const = 0;

};
