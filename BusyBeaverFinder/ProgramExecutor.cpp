//
//  ProgramExecutor.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 25/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "ProgramExecutor.h"

#include <iostream>

void ProgramExecutor::dumpExecutionState() const {
    getData().dump();

    std::cout << "Run summary: ";
    getRunSummary().dump();
    getRunSummary().dumpCondensed();

    std::cout << "Meta-run summary: ";
    getMetaRunSummary().dump();
    getMetaRunSummary().dumpCondensed();
}
