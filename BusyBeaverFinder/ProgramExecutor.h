//
//  ProgramExecutor.h
//  BusyBeaverFinder
//
//  Created by Erwin on 25/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef ProgramExecutor_h
#define ProgramExecutor_h

#include "Data.h"
#include "RunSummary.h"
#include "ProgramBlock.h"

/* Abstract data type. Interface to hang detectors
 */
class ProgramExecutor {

public:
    virtual const Data& getData() const = 0;

    virtual const RunSummary& getRunSummary() const = 0;
    virtual const RunSummary& getMetaRunSummary() const = 0;

//    virtual ProgramBlock* getProgramBlock(int index) const = 0;
//    virtual int numProgramBlocks() const = 0;
};

#endif /* ProgramExecutor_h */
