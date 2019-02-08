//
//  DataTracker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 07/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "DataTracker.h"

#include <iostream>
#include <stdio.h>
#include <string.h>

#include "Data.h"
#include "Utils.h"

DataTracker::DataTracker(Data& data) : _data(data) {
    _snapShotA.buf = new int[data.getSize()];
    _snapShotB.buf = new int[data.getSize()];
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

    // TODO: A full copy is not needed when old snapshot is reused. In this case, a smart partial
    // update suffices.
    int *buf = _newSnapShotP->buf;
    memcpy(buf, _data.getDataBuffer(), sizeof(int) * _data.getSize());

    _newSnapShotP->dataP = _data.getDataPointer();
    _newSnapShotP->minVisitedP = _data.getMinVisitedP();
    _newSnapShotP->maxVisitedP = _data.getMaxVisitedP();

    _data.resetVisitedBounds();
}

// Checks if a change in data value is impactful. An impactful change is one that, when carried out
// repeatedly, will impact a TURN evaluation. I.e. a data value moved closer to zero (or its value
// was zero and not anymore). In the macro, x is the old value and y the new value.
#define IMPACTFUL_CHANGE(x, y) ((x <= 0 && y > x) || (x >= 0 && y < x))

SnapShotComparison  DataTracker::compareToSnapShot() {
    SnapShotComparison result = SnapShotComparison::UNCHANGED;

    int *p1 = _data.getMinVisitedP();
    int *p2 = _newSnapShotP->buf + (p1 - _data.getDataBuffer());
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

bool DataTracker::compareSnapShotDeltas() {
    long _deltaNew = _data.getMaxVisitedP() - _data.getMinVisitedP();
    long _deltaOld = _newSnapShotP->maxVisitedP - _newSnapShotP->minVisitedP;

    if (_deltaNew != _deltaOld) {
        // The number of cells visited differ
        return false;
    }

    long shift = _data.getDataPointer() - _newSnapShotP->dataP;

    int *oldBeforeP = _oldSnapShotP->buf + (_newSnapShotP->minVisitedP - _data.getDataBuffer());
    int *oldAfterP = _newSnapShotP->buf + (_newSnapShotP->minVisitedP - _data.getDataBuffer());
    int *newBeforeP = _newSnapShotP->buf + (_data.getMinVisitedP() - _data.getDataBuffer());
    // Expanded for clarity
    int *newAfterP = _data.getDataBuffer() + (_data.getMinVisitedP() - _data.getDataBuffer());

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
        while (newAfterP <= _data.getMaxDataP()) {
            if (*newAfterP != 0) {
                return false;
            }
            newAfterP++;
        }
    }
    else if (shift < 0) {
        // Check that the newly visited values were all zeros
        newBeforeP = _newSnapShotP->buf + (_data.getMinVisitedP() - _data.getDataBuffer());
        while (shift < 0) {
            if (*newBeforeP != 0) {
                return false;
            }
            newBeforeP++; // Note: The increase is intentional
            shift++;
        }

        // Check that there are only zeros ahead. Other values may break the repetitive behavior
        newAfterP = _data.getMinVisitedP() - 1;
        while (newAfterP >= _data.getMinDataP()) {
            if (*newAfterP != 0) {
                return false;
            }
            newAfterP--;
        }
    }

    return true;
}

void DataTracker::dump() {
    if (_newSnapShotP != nullptr) {
        std::cout << "SNAP1: min = " << (_newSnapShotP->minVisitedP - _data.getDataBuffer())
        << ", p = " << (_newSnapShotP->dataP - _data.getDataBuffer())
        << ", max = " << (_newSnapShotP->maxVisitedP - _data.getDataBuffer())
        << std::endl;
        dumpDataBuffer(_newSnapShotP->buf,
                       _newSnapShotP->buf + (_newSnapShotP->dataP - _data.getDataBuffer()),
                       _data.getSize());
    }

    if (_oldSnapShotP != nullptr) {
        std::cout << "SNAP2: min = " << (_oldSnapShotP->minVisitedP - _data.getDataBuffer())
        << ", p = " << (_oldSnapShotP->dataP - _data.getDataBuffer())
        << ", max = " << (_oldSnapShotP->maxVisitedP - _data.getDataBuffer())
        << std::endl;
        dumpDataBuffer(_oldSnapShotP->buf,
                       _oldSnapShotP->buf + (_oldSnapShotP->dataP - _data.getDataBuffer()),
                       _data.getSize());
    }
}
