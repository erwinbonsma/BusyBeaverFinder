//
//  HangChecker.h
//  BusyBeaverFinder
//
//  Created by Erwin on 27/11/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#pragma once

#include "ExecutionState.h"

/* Responsible for checking/proofing that a program hangs. The checker is activated after analysis
 * has shown that the program exhibits behavior characterstic for a specific type of hang. The
 * checker should verify that the program cannot escape from the hang behavior. This decision may
 * not be instantaneous. The checker may inspect the execution state at different moments in the
 * future (this is more efficient than the checker predicting the future state by simulating the
 * program execution).
 */
class HangChecker {

public:
    /* Checks if the program is proven to hang. It can return MAYBE. In this case the hang
     * detector is responsible for invoking the check again at a later point in the program's
     * execution, after verifying that the program still exhibits the same behavior.
     */
    virtual Trilian proofHang(const ExecutionState& _execution) = 0;
};
