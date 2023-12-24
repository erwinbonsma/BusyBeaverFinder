//
//  DataDeltas.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 22/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "DataDeltas.h"

void DataDeltas::updateBounds() const {
    _minDp = 0;
    _maxDp = 0;

    for (const DataDelta &dd : _dataDeltas) {
        int dp = dd.dpOffset();
        _minDp = std::min(_minDp, dp);
        _maxDp = std::max(_maxDp, dp);
    }
    _dpBoundsValid = true;
}

int DataDeltas::minDpOffset() const {
    if (!_dpBoundsValid) {
        updateBounds();
    }
    return _minDp;
}

int DataDeltas::maxDpOffset() const {
    if (!_dpBoundsValid) {
        updateBounds();
    }
    return _maxDp;
}

int DataDeltas::deltaAt(int dpOffset) const {
    // Find existing delta record, if any
    for (const DataDelta &dd : _dataDeltas) {
        if (dd.dpOffset() == dpOffset) {
            return dd.delta();
        }
    }

    return 0;
}

int DataDeltas::updateDelta(int dpOffset, int delta) {
    auto iter = _dataDeltas.begin();

    _dpBoundsValid = false; // Invalidate cache

    // Find existing delta record, if any
    while (iter != _dataDeltas.end() && iter->dpOffset() != dpOffset) {
        iter++;
    }

    if (iter == _dataDeltas.end()) {
        // No existing record was found, so create one.
        iter = _dataDeltas.emplace(iter, dpOffset);
    }

    if (iter->changeDelta(delta)) {
        // This change cancelled out previous changes. Remove the entry to reflect this
        if (_dataDeltas.size() > 0) {
            *iter = _dataDeltas.back();
            _dataDeltas.pop_back();
        }

        return 0;
    } else {
        return iter->delta();
    }
}

void DataDeltas::addDelta(int dpOffset, int delta) {
    _dpBoundsValid = false;
    _dataDeltas.push_back(DataDelta(dpOffset, delta));
}

std::ostream &operator<<(std::ostream &os, const DataDeltas& dataDeltas) {
    bool isFirst = true;
    for (DataDelta dd : dataDeltas) {
        if (!isFirst) {
            os << " ";
        } else {
            isFirst = false;
        }
        os << dd.delta() << "@" << dd.dpOffset();
    }

    return os;
}
