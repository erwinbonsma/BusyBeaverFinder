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
class ProgressTracker;

class FastExecutor {
    int* _data;
    int _dataBufSize;

    int* _minDataP;
    int* _midDataP;
    int* _maxDataP;

    int* _dataP;

    ProgressTracker* _tracker;
public:
    FastExecutor(int dataSize);
    ~FastExecutor();

    void setProgressTracker(ProgressTracker* tracker) { _tracker = tracker; }

    void execute(ProgramBlock *programBlock, int maxSteps);

    void dump();
};

#endif /* FastExecutor_h */
