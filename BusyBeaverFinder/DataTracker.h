//
//  DataTracker.h
//  BusyBeaverFinder
//
//  Created by Erwin on 07/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef DataTracker_h
#define DataTracker_h

#include <stdio.h>

struct SnapShot {
    int *buf;
    int *dataP;

    // Delimits the range of values that have been visisted since the last snapshot was taken.
    // These are the date values that may have impacted program execution since then.
    int *minVisitedP, *maxVisitedP;
};

enum class SnapShotComparison : char {
    // Data did not change
    UNCHANGED = 0,
    // Data changed, but diverging from zero (which will not impact TURN evaluation)
    DIVERGING = 1,
    // Data changed in a way that can impact program flow (i.e. towards zero)
    IMPACTFUL = 2
};

class Data;

class DataTracker {
    Data& _data;

    // Two snapshots. These should not be used directly, instead use oldSnapShot and newSnapShot
    SnapShot _snapShotA;
    SnapShot _snapShotB;

    SnapShot *_oldSnapShotP;
    SnapShot *_newSnapShotP;
public:
    DataTracker(Data& data);
    ~DataTracker();

    void reset();

    SnapShot* getOldSnapShot() { return _oldSnapShotP; }
    SnapShot* getNewSnapShot() { return _newSnapShotP; }

    void captureSnapShot();
    SnapShotComparison compareToSnapShot();

    // Returns "true" if a hang is detected
    bool compareSnapShotDeltas();

    void dump();
};

#endif /* DataTracker_h */
