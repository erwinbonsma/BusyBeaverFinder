//
//  Data.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef Data_h
#define Data_h

#include <stdio.h>

#include "Consts.h"
#include "Enums.h"

struct SnapShot {
    int *buf;
    int *dataP;

    // Delimits the range of values that have been visisted since the last snapshot was taken.
    // These are the date values that may have impacted program execution since then.
    int *minVisitedP, *maxVisitedP;
};

class Data {
    int *_dataP;
    int *_minDataP, *_midDataP, *_maxDataP;

    // Delimits the data cells that have been visisted (since the last snapshot was taken)
    int *_minVisitedP, *_maxVisitedP;

    // Array with data values
    int *_data;

    DataOp *_undoP = nullptr;
    // Undo-stack for data operations
    DataOp *_undoStack = nullptr;

/* Hang Detection 1 collects effective data instruction. It collapses subsequent data instructions
 * that cancel each other out, e.g. DEC after INC, SHL after SHR. If at the end of the hang sample
 * period there are no effective instructions, it concludes that the program hangs.
 */
#ifdef HANG_DETECTION1
    DataOp *_effectiveP = nullptr;
    // Stack with effective instruction in current hang sample period
    DataOp *_effective = nullptr;
#endif

    // Two snapshots. These should not be used directly, instead use oldSnapShot and newSnapShot
    SnapShot _snapShotA;
    SnapShot _snapShotB;

    SnapShot *_oldSnapShotP;
    SnapShot *_newSnapShotP;
    bool _significantValueChange;

    void dumpDataBuffer(int* buf, int* dataP);

public:
    Data(int size);
    ~Data();

    void setStackSize(int size);
    void setHangSamplePeriod(int period);

    long getSize() { return _maxDataP - _minDataP + 1; }

    void resetHangDetection();

    /* Returns "true" if there have been effective data operations since the last snapshot.
     * Operations are effective if they do not cancel out the previous (effective) operation.
     * E.g. a DEC that follows an INC is not effective.
     *
     * TODO: Check why this detects hangs not detected by the snapshot-based hang detection.
     */
    bool effectiveDataOperations();

    /* True if one or more values since last snapshot became zero, or moved away from zero.
     */
    bool significantValueChange() { return _significantValueChange; }

    SnapShot* getOldSnapShot() { return _oldSnapShotP; }
    SnapShot* getNewSnapShot() { return _newSnapShotP; }

    void captureSnapShot();
    SnapShotComparison compareToSnapShot();
    bool areSnapShotDeltasAreIdentical();

    int* getDataPointer() { return _dataP; }
    int val() { return *_dataP; }

    void inc();
    void dec();
    bool shr();
    bool shl();

    void undo(int num);

    void dump();
    void dumpStack();
    void dumpSettings();
    void dumpHangInfo();
};

#endif /* Data_h */
