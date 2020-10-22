//
//  DataDeltas.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 22/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "DataDeltas.h"

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
