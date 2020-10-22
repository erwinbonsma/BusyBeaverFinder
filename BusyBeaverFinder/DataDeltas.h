//
//  DataDeltas.h
//  BusyBeaverFinder
//
//  Created by Erwin on 22/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef DataDeltas_h
#define DataDeltas_h

#include <vector>

class DataDelta {
    int _dpOffset;
    int _delta;

public:
    DataDelta() { _dpOffset = 0; _delta = 0; }
    DataDelta(int dpOffset) { _dpOffset = dpOffset; _delta = 0; }
    DataDelta(int dpOffset, int delta) { _dpOffset = dpOffset; _delta = delta; }

    // Returns "true" if this results in a zero change (so that the delta can be removed).
    bool changeDelta(int delta) { _delta += delta; return _delta == 0; }

    // Specifies the position of the data value relative to an assumed reference point
    int dpOffset() const { return _dpOffset; }

    // Specifies how much this value changes
    int delta() const { return _delta; }
};

class DataDeltas {
    std::vector<DataDelta> _dataDeltas;

    typedef std::vector<DataDelta>::const_iterator const_iterator;

public:
    void clear() { _dataDeltas.clear(); }

    int numDeltas() const { return (int)_dataDeltas.size(); }
    const DataDelta& dataDelta(int index) const { return _dataDeltas[index]; }

    int deltaAt(int dpOffset) const;

    // Updates and returns the effective delta at the specified data position.
    int updateDelta(int dpOffset, int delta);

    const_iterator begin() { return _dataDeltas.cbegin(); }
    const_iterator end() { return _dataDeltas.cend(); }
};

#endif /* DataDeltas_h */
