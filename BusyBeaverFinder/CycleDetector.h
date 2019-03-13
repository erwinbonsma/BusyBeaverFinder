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

#include "Types.h"

typedef char CycleInstruction;

class CycleDetector {
    // Stack of recently requested/executed operations
    CycleInstruction* _opsHistory = nullptr;

    // Pointer to current top of the stack
    CycleInstruction* _opsHistoryP = nullptr;
    CycleInstruction* _opsHistoryMaxP = nullptr;

    // Temporary buffer needed by findPeriod function
    int* _findPeriodBuf;

public:
    CycleDetector();
    ~CycleDetector();

    void setHangSamplePeriod(int period);

    void recordInstruction(CycleInstruction instruction) {
        *(_opsHistoryP++) = instruction;
//        std::cout << "op[" << (_opsHistoryP - _opsHistory) << "]=" << (int)op << std::endl;
        assert(_opsHistoryP < _opsHistoryMaxP);
    }
    int getNumRecordedInstructions() { return (int)(_opsHistoryP - _opsHistory); }
    void clearInstructionHistory() { _opsHistoryP = _opsHistory; }

    int getCyclePeriod();

    void dump();
};


#endif /* CycleDetector_h */
