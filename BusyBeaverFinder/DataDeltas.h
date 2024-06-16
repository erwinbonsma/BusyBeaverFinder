//
//  DataDeltas.h
//  BusyBeaverFinder
//
//  Created by Erwin on 22/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//
#pragma once

#include <vector>
#include <iostream>

#include "RunSummary.h"

class DataDelta {
    int _dpOffset;
    int _delta;

public:
    DataDelta() { _dpOffset = 0; _delta = 0; }
    DataDelta(int dpOffset) { _dpOffset = dpOffset; _delta = 0; }
    DataDelta(int dpOffset, int delta) { _dpOffset = dpOffset; _delta = delta; }
    DataDelta(const DataDelta&) = default;

    // Returns "true" if this results in a zero change (so that the delta can be removed).
    bool changeDelta(int delta) { _delta += delta; return _delta == 0; }

    // Specifies the position of the data value relative to an assumed reference point
    int dpOffset() const { return _dpOffset; }

    // Specifies how much this value changes
    int delta() const { return _delta; }
};

class DataDeltas {
    // TODO: Store using map?
    // Note: This then also changes iteration
    std::vector<DataDelta> _dataDeltas;

    // Cache bounds
    mutable int _minDp;
    mutable int _maxDp;
    mutable bool _dpBoundsValid;

    typedef std::vector<DataDelta>::const_iterator const_iterator;

    void updateBounds() const;

public:
    DataDeltas() = default;
    DataDeltas(const DataDeltas&) = delete;
    DataDeltas& operator=(const DataDeltas&) = delete;

    void clear() { _dataDeltas.clear(); _dpBoundsValid = false; }

    int size() const { return (int)_dataDeltas.size(); }
    const DataDelta& operator[](int index) const { return _dataDeltas[index]; }
    int minDpOffset() const;
    int maxDpOffset() const;

    int deltaAt(int dpOffset) const;

    // Updates and returns the effective delta at the specified data position.
    int updateDelta(int dpOffset, int delta);

    // Adds the given delta. It should only be used when it was first checked that no delta exists
    // yet for this offset (using deltaAt). It is then more efficient than using updateDelta.
    void addDelta(int dpOffset, int delta);

    // Adds the contribution from a range of ProgramBlocks from RunHistory.
    // Note: Cannot iterate in reverse order, as this messes with DP updates.
    template <typename Iterator>
    void bulkAdd(Iterator begin, Iterator end, int dp0) {
        int dp = dp0;
        Iterator it = begin;
        while (it != end) {
            const ProgramBlock *pb = *it++;
            if (pb->isDelta()) {
                updateDelta(dp, pb->getInstructionAmount());
            } else {
                dp += pb->getInstructionAmount();
            }
        }
    }

    template <typename Iterator, typename Predicate>
    void bulkAdd(Iterator begin, Iterator end, int dp0, Predicate pred) {
        int dp = dp0;
        Iterator it = begin;
        while (it != end) {
            const ProgramBlock *pb = *it++;
            if (pb->isDelta()) {
                if (pred(dp)) {
                    updateDelta(dp, pb->getInstructionAmount());
                }
            } else {
                dp += pb->getInstructionAmount();
            }
        }
    }

    const_iterator begin() const { return _dataDeltas.cbegin(); }
    const_iterator end() const { return _dataDeltas.cend(); }
    const_iterator erase(const_iterator it) { return _dataDeltas.erase(it); }
};

std::ostream &operator<<(std::ostream &os, const DataDelta& dataDelta);
std::ostream &operator<<(std::ostream &os, const DataDeltas& dataDeltas);
