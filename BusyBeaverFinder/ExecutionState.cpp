//
//  ExecutionState.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 25/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "ExecutionState.h"

#include "Data.h"
#include "RunSummary.h"
#include "RunBlockTransitions.h"

#include <iostream>

void ExecutionState::dumpExecutionState() const {
    getData().dump();

    std::cout << "Run summary: ";
//    getRunSummary().dump();
    getRunSummary().dumpCondensed();

    std::cout << "Run-block transitions:" << std::endl;
    getRunBlockTransitions().dump();

    std::cout << "Meta-run summary: ";
//    getMetaRunSummary().dump();
    getMetaRunSummary().dumpCondensed();

    std::cout << "Meta-meta-run summary: ";
//    getMetaRunSummary().dump();
    getMetaMetaRunSummary().dumpCondensed();
}
