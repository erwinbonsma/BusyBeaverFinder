//
//  CycleDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 07/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef CycleDetector_h
#define CycleDetector_h

#include <assert.h>
#include <iostream>
#include <stdio.h>

#include "Enums.h"

class CycleDetector {
    // Stack of recently requested/executed operations
    Op* _opsHistory = nullptr;

    // Pointer to current top of the stack
    Op* _opsHistoryP = nullptr;
    Op* _opsHistoryMaxP = nullptr;

    // Temporary buffer needed by findPeriod function
    int* _findPeriodBuf;

public:
    CycleDetector();
    ~CycleDetector();

    void setHangSamplePeriod(int period);

    void recordInstruction(Op op) {
        *(_opsHistoryP++) = op;
//        std::cout << "op[" << (_opsHistoryP - _opsHistory) << "]=" << (int)op << std::endl;
        assert(_opsHistoryP < _opsHistoryMaxP);
    }

    void clearInstructionHistory();
    int getCyclePeriod();

    void dump();
};


#endif /* CycleDetector_h */
