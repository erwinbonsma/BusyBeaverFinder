//
//  FastExecutor.h
//  BusyBeaverFinder
//
//  Created by Erwin on 22/05/19.
//  Copyright © 2019 Erwin Bonsma
//

#ifndef FastExecutor_h
#define FastExecutor_h

#include <stdio.h>

class ProgramBlock;

class FastExecutor {
    int* _data;
    int _dataBufSize;

    int* _minDataP;
    int* _midDataP;
    int* _maxDataP;

    int* _dataP;

public:
    FastExecutor(int dataSize);
    ~FastExecutor();

    // Returns the number of executed steps. Returns -1 in case of an error (data full). When it is
    // equal or larger than maxSteps, it is an assumed hang.
    int execute(ProgramBlock *programBlock, int maxSteps);

    void dump();
};

#endif /* FastExecutor_h */
