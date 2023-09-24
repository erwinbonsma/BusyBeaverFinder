//
//  Utils.h
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include <set>
#include <string>
#include <vector>
#include <istream>

#include "Types.h"

#define PROGRAM_POINTERS_MATCH(pp1, pp2) ( \
    pp1.p.col == pp2.p.col && \
    pp1.p.row == pp2.p.row && \
    pp1.dir == pp2.dir \
)

bool isPowerOfTwo(int val);
int makePowerOfTwo(int val);

inline int normalizedMod(int operand, int modulus) {
    int val = operand % modulus;
    return (val >= 0) ? val : val + abs(modulus);
}

inline int sign(int val) {
    return val > 0 ? 1 : (val < 0 ? -1 : 0);
}

InstructionPointer nextInstructionPointer(ProgramPointer insP);

void calculateZArray(const char* input, int* output, int len);
int findPeriod(const char* input, int* buf, int len);

// Checks if the "input" array, containing "len" elements, ends with a repeated sequence.
// It will return the length of this sequence if one is found, and zero otherwise.
//
// Examples:
// aaaaabcabc => 3
// abcdabc => 0
// abcdee => 1
int findRepeatedSequence(const int* input, int* buf, int len);

// Returns true if any combination of deltas (with repeats) can sum to target value.
bool deltasCanSumTo(std::set<int> deltas, int target);

void loadResumeStackFromStream(std::istream &input, std::vector<Ins> &resumeStack);
bool loadResumeStackFromFile(std::string inputFile, std::vector<Ins> &resumeStack);

void dumpDataBuffer(int* buf, int* dataP, int size);
void dumpInstructionStack(const std::vector<Ins> &stack);

template <class T>
class ProxyIterator {
private:
    T& _container;
public:
    class iterator {
    private:
        typename T::const_iterator it;
    public:
        iterator(typename T::const_iterator it) : it(it) {}
        iterator operator++() { return ++it; }
        bool operator!=(const iterator & other) { return it != other.it; }
        typename T::key_type operator*() const { return *it; }
    };

    ProxyIterator(T& container) : _container(container) {}
    iterator begin() const { return iterator(_container.begin()); }
    iterator end() const { return iterator(_container.end()); }
};

template <class T>
ProxyIterator<T> makeProxyIterator(T& container) {
    return ProxyIterator<T>(container);
}

template <class MapType>
class MapKeyIterator {
private:
    MapType& map;
public:
    class iterator {
    private:
        typename MapType::const_iterator it;
    public:
        iterator(typename MapType::const_iterator it) : it(it) {}
        iterator operator++() { return ++it; }
        bool operator!=(const iterator & other) { return it != other.it; }
        typename MapType::key_type operator*() const { return it->first; } // Return the key
    };

    MapKeyIterator(MapType& m) : map(m) {}
    iterator begin() const { return iterator(map.begin()); }
    iterator end() const { return iterator(map.end()); }
};

template <class MapType>
MapKeyIterator<MapType> makeKeyIterator(MapType& m) {
    return MapKeyIterator<MapType>(m);
}
