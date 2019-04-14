//
//  DataTracker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 07/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "DataTracker.h"

#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <string.h>

#include "Data.h"
#include "Utils.h"

void DataTracker::initSnapShot(SnapShot& snapshot) {
    snapshot.buf = new int[_data.getSize()];
    snapshot.minBoundP = _data.getMinBoundP();
    snapshot.maxBoundP = _data.getMaxBoundP();

    memcpy(snapshot.buf, _dataBufP, sizeof(int) * _data.getSize());
}

DataTracker::DataTracker(Data& data) : _data(data) {
    _dataBufP = data.getDataBuffer();

    initSnapShot(_snapShotA);
    initSnapShot(_snapShotB);
}

DataTracker::~DataTracker() {
    delete[] _snapShotA.buf;
    delete[] _snapShotB.buf;
}

void DataTracker::reset() {
    _oldSnapShotP = nullptr;
    _newSnapShotP = nullptr;
}

void DataTracker::captureSnapShot() {
    if (_newSnapShotP == nullptr) {
        _newSnapShotP = &_snapShotA;
    }
    else if (_oldSnapShotP == nullptr) {
        _oldSnapShotP = _newSnapShotP;
        _newSnapShotP = &_snapShotB;
    }
    else {
        SnapShot *tmp = _newSnapShotP;
        _newSnapShotP = _oldSnapShotP;
        _oldSnapShotP = tmp;
    }

    int *minP = ((_data.getMinBoundP() < _newSnapShotP->minBoundP)
                 ? _data.getMinBoundP() : _newSnapShotP->minBoundP);
    int *maxP = ((_data.getMaxBoundP() > _newSnapShotP->maxBoundP)
                 ? _data.getMaxBoundP() : _newSnapShotP->maxBoundP);

    // Copy the range of values that were non-zero or are non-zero.
    if (minP <= maxP) {
        int offset = (int)(minP - _dataBufP);
        int len = (int)(maxP - minP + 1);

        memcpy(_newSnapShotP->buf + offset, minP, sizeof(int) * len);
    }

    _newSnapShotP->dataP = _data.getDataPointer();
    _newSnapShotP->minVisitedP = _data.getMinVisitedP();
    _newSnapShotP->maxVisitedP = _data.getMaxVisitedP();
    _newSnapShotP->minBoundP = _data.getMinBoundP();
    _newSnapShotP->maxBoundP = _data.getMaxBoundP();

    _data.resetVisitedBounds();
}

// Checks if a change in data value is impactful. An impactful change is one that, when carried out
// repeatedly, will impact a TURN evaluation. I.e. a data value moved closer to zero (or its value
// was zero and not anymore). In the macro, x is the old value and y the new value.
#define IMPACTFUL_CHANGE(x, y) ((x <= 0 && y > x) || (x >= 0 && y < x))

SnapShotComparison  DataTracker::compareToSnapShot() {
    SnapShotComparison result = SnapShotComparison::UNCHANGED;

    int *p1 = _data.getMinVisitedP();
    int *p2 = _newSnapShotP->buf + (p1 - _dataBufP);
    do {
        if (*p1 != *p2) {
            if (IMPACTFUL_CHANGE(*p2, *p1)) {
                return SnapShotComparison::IMPACTFUL;
            } else {
                result = SnapShotComparison::DIVERGING;
            }
        }
        p1++;
        p2++;
    } while (p1 <= _data.getMaxVisitedP());

    return result;
}

/* Checks for hangs that are periodic.
 *
 * This check should be invoked with two snapshots taken at identical spots in the program (same PP,
 * including direction) separated in time by the periodic interval suggested by the cycle detector.
 *
 * It checks if during both periods the same data changes are made, possibly shifted.
 */
bool DataTracker::periodicHangDetected() {
    long _deltaNew = _data.getMaxVisitedP() - _data.getMinVisitedP();
    long _deltaOld = _newSnapShotP->maxVisitedP - _newSnapShotP->minVisitedP;

    if (_deltaNew != _deltaOld) {
        // The number of cells visited differ
        return false;
    }

    assert(_dataBufP == _data.getDataBuffer());
    long shift = _data.getDataPointer() - _newSnapShotP->dataP;

    int *oldBeforeP = _oldSnapShotP->buf + (_newSnapShotP->minVisitedP - _dataBufP);
    int *oldAfterP = _newSnapShotP->buf + (_newSnapShotP->minVisitedP - _dataBufP);
    int *newBeforeP = _newSnapShotP->buf + (_data.getMinVisitedP() - _dataBufP);
    // Expanded for clarity
    int *newAfterP = _dataBufP + (_data.getMinVisitedP() - _dataBufP);

    do {
        if (
            *oldBeforeP != *newBeforeP ||
            *oldAfterP != *newAfterP
        ) {
            // When there is a change in values, only detect hangs when the data pointer did not
            // move.
            if (shift != 0) {
                return false;
            }

            // The delta between both snapshots must be the same (i.e. both loops should have the
            // same effect)
            if ((*newAfterP - *newBeforeP) != (*oldAfterP - *oldBeforeP)) {
                return false;
            }

            // Neither of the deltas should not have gone through zero (as this may result in
            // different execution flows, possibly breaking the loop).
            //
            // Note, this check does not detect the following: a value is positive at the start and
            // end of its snapshot, but actually was zero in between. It is assumed that when that
            // happens, this happens in both cases. As this check is only invoked after a periodic
            // loop is detected, it is very unlikely that this assumption is invalid. Should this
            // happens, this check can be refined.
            if (
                (*newAfterP == 0) ||
                (*newAfterP > 0 && (*oldAfterP <= 0 || *newBeforeP <= 0 || *oldBeforeP <= 0)) ||
                (*newAfterP < 0 && (*oldAfterP >= 0 || *newBeforeP >= 0 || *oldBeforeP >= 0))
            ) {
                return false;
            }

            // The change should be moving away from zero. Otherwise the loop may be broken once
            // the value becomes zero.
            if (
                (*newAfterP > 0 && (*newAfterP - *newBeforeP) < 0) ||
                (*newAfterP < 0 && (*newAfterP - *newBeforeP) > 0)
            ) {
                return false;
            }
        }

        oldBeforeP++;
        newBeforeP++;
        oldAfterP++;
        newAfterP++;
    } while (newAfterP <= _data.getMaxVisitedP());

    if (shift > 0) {
        // Check that the newly visited values were all zeros
        newBeforeP -= shift;
        while (shift > 0) {
            if (*newBeforeP != 0) {
                return false;
            }
            newBeforeP++;
            shift--;
        }

        // Check that there are only zeros ahead. Other values may break the repetitive behavior
        if (_data.getMaxVisitedP() < _data.getMaxBoundP()) {
            return false;
        }
    }
    else if (shift < 0) {
        // Check that the newly visited values were all zeros
        newBeforeP = _newSnapShotP->buf + (_data.getMinVisitedP() - _dataBufP);
        while (shift < 0) {
            if (*newBeforeP != 0) {
                return false;
            }
            newBeforeP++; // Note: The increase is intentional
            shift++;
        }

        // Check that there are only zeros ahead. Other values may break the repetitive behavior
        if (_data.getMinVisitedP() > _data.getMinBoundP()) {
            return false;
        }
    }

    return true;
}

bool DataTracker::sweepHangDetected(int* sweepMidTurningPoint) {
    // Check if there was an impactful change from the old to the new snapshot
    int *p = _oldSnapShotP->buf + (_newSnapShotP->minBoundP - _dataBufP);
    int *q = _newSnapShotP->buf + (_newSnapShotP->minBoundP - _dataBufP);
    int *pEnd = _oldSnapShotP->buf + (_newSnapShotP->maxBoundP - _dataBufP);
    do {
        if (IMPACTFUL_CHANGE(*p, *q)) {
            bool isAllowed = false;

            if (
                sweepMidTurningPoint != nullptr &&
                (p - _oldSnapShotP->buf) == (sweepMidTurningPoint - _dataBufP)
            ) {
                // The mid-sequence turning point may (briefly) become zero
                isAllowed = true;
            }

            if (
                (p - _oldSnapShotP->buf) < (_oldSnapShotP->minBoundP - _dataBufP) ||
                (p - _oldSnapShotP->buf) > (_oldSnapShotP->maxBoundP - _dataBufP)
            ) {
                // Zero cells outside the data bounds may be set (extending the sequence)
                isAllowed = true;
            }

            if (!isAllowed) {
                return false;
            }
        }
        p++;
        q++;
    } while (p <= pEnd);

    // Check if there was an impactful change from the new snapshot to the current data state.
    p = _newSnapShotP->buf + (_data.getMinBoundP() - _dataBufP);
    q = _data.getMinBoundP();
    pEnd = _newSnapShotP->buf + (_data.getMaxBoundP() - _dataBufP);
    do {
        if (IMPACTFUL_CHANGE(*p, *q)) {
            bool isAllowed = false;

            // Check if it's a mid-sequence turning point. This one may (briefly) become zero
            if (q == sweepMidTurningPoint) {
                isAllowed = true;
            }

            if (
                (p - _newSnapShotP->buf) < (_newSnapShotP->minBoundP - _dataBufP) ||
                (p - _newSnapShotP->buf) > (_newSnapShotP->maxBoundP - _dataBufP)
            ) {
                // Zero cells outside the data bounds may be set (extending the sequence)
                isAllowed = true;
            }

            if (!isAllowed) {
                return false;
            }
        }
        p++;
        q++;
    } while (p <= pEnd);

    return true;
}

/* Checks for glider hangs.
 *
 * This check should be invoked with two snapshots.
 */
bool DataTracker::gliderHangDetected() {
    long _deltaNew = _data.getMaxVisitedP() - _data.getMinVisitedP();
    long _deltaOld = _newSnapShotP->maxVisitedP - _newSnapShotP->minVisitedP;

    if (_deltaNew != _deltaOld) {
        // The number of cells visited differ
        return false;
    }

    assert(_dataBufP == _data.getDataBuffer());
    long shift = _data.getDataPointer() - _newSnapShotP->dataP;

    if (shift == 0) {
        // A glider is expected to move
        return false;
    }

    int *oldBeforeP = _oldSnapShotP->buf + (_oldSnapShotP->minVisitedP - _dataBufP);
    int *oldAfterP = _newSnapShotP->buf + (_newSnapShotP->minVisitedP - _dataBufP);
    int *newBeforeP = oldAfterP;
    // Expanded for clarity
    int *newAfterP = _dataBufP + (_data.getMinVisitedP() - _dataBufP);

    int numDeltas = 0;

    do {
        if (
            *oldBeforeP != *newBeforeP ||
            *oldAfterP != *newAfterP
        ) {
            // The delta of the latest change must be at least as large as the previous delta
            if (abs(*newAfterP - *newBeforeP) < abs(*oldAfterP - *oldBeforeP)) {
                return false;
            }

            // Neither of the deltas should not have gone through zero (as this may result in
            // different execution flows, possibly breaking the loop).
            if (
                (*newAfterP == 0) ||
                (*newAfterP > 0 && (*oldAfterP <= 0 || *newBeforeP <= 0 || *oldBeforeP <= 0)) ||
                (*newAfterP < 0 && (*oldAfterP >= 0 || *newBeforeP >= 0 || *oldBeforeP >= 0))
            ) {
                return false;
            }

            // The change should be moving away from zero. Otherwise the loop may be broken once
            // the value becomes zero.
            if (
                (*newAfterP > 0 && (*newAfterP - *newBeforeP) < 0) ||
                (*newAfterP < 0 && (*newAfterP - *newBeforeP) > 0)
            ) {
                return false;
            }

            if (++numDeltas > 1) {
                // Only one loop-counter is expected (for simple gliders)
                return false;
            }
        }

        oldBeforeP++;
        newBeforeP++;
        oldAfterP++;
        newAfterP++;
    } while (newAfterP <= _data.getMaxVisitedP());

    if (shift > 0) {
        // Check that the newly visited values were all zeros
        newBeforeP = _newSnapShotP->buf + (_data.getMaxVisitedP() - _dataBufP);
        while (shift > 0) {
            if (*newBeforeP != 0) {
                return false;
            }
            newBeforeP++;
            shift--;
        }

        // Check that there are only zeros ahead. Other values may break the repetitive behavior
        if (_data.getMaxVisitedP() < _data.getMaxBoundP()) {
            return false;
        }
    }
    else if (shift < 0) {
        // Check that the newly visited values were all zeros
        newBeforeP = _newSnapShotP->buf + (_data.getMinVisitedP() - _dataBufP);
        while (shift < 0) {
            if (*newBeforeP != 0) {
                return false;
            }
            newBeforeP++; // Note: The increase is intentional
            shift++;
        }

        // Check that there are only zeros ahead. Other values may break the repetitive behavior
        if (_data.getMinVisitedP() > _data.getMinBoundP()) {
            return false;
        }
    }

    return true;
}

void DataTracker::dump() {
    if (_newSnapShotP != nullptr) {
        std::cout << "SNAP1: min = " << (_newSnapShotP->minVisitedP - _dataBufP)
        << ", p = " << (_newSnapShotP->dataP - _dataBufP)
        << ", max = " << (_newSnapShotP->maxVisitedP - _dataBufP)
        << std::endl;
        dumpDataBuffer(_newSnapShotP->buf + (_newSnapShotP->minBoundP - _dataBufP),
                       _newSnapShotP->buf + (_newSnapShotP->dataP - _dataBufP),
                       (int)(_newSnapShotP->maxBoundP - _newSnapShotP->minBoundP + 1));
    }

    if (_oldSnapShotP != nullptr) {
        std::cout << "SNAP2: min = " << (_oldSnapShotP->minVisitedP - _dataBufP)
        << ", p = " << (_oldSnapShotP->dataP - _dataBufP)
        << ", max = " << (_oldSnapShotP->maxVisitedP - _dataBufP)
        << std::endl;
        dumpDataBuffer(_oldSnapShotP->buf + (_oldSnapShotP->minBoundP - _dataBufP),
                       _oldSnapShotP->buf + (_oldSnapShotP->dataP - _dataBufP),
                       (int)(_oldSnapShotP->maxBoundP - _oldSnapShotP->minBoundP + 1));
    }
}
