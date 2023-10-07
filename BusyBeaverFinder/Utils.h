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
template <typename T>
int findRepeatedSequence(const T* input, int* buf, int len);

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

template <class Iter>
class Range {
    Iter _b;
    Iter _e;
public:
    Range(Iter b, Iter e) : _b(b), _e(e) {}

    Iter begin() { return _b; }
    Iter end() { return _e; }
};

template <class Container>
Range<typename Container::const_iterator>
makeRange(Container& c, size_t begin, size_t end) {
    return Range<typename Container::const_iterator> (c.cbegin() + begin, c.cbegin() + end);
}

// This implementation is very similar to that of findPeriod. The only three changes are:
// - Reversed iteration direction over the input sequence (moving from end towards start)
// - Changed termination criterion and return value
// - Only iterate over half the sequence (as it will not find a match anymore beyond that)
template <typename T>
int findRepeatedSequence(const T* input, int* buf, int len) {
    int lastpos = len - 1;
    int halflen = len / 2;
    int l, r;
    l = r = 0;

    buf[0] = 0; // Should not be used
    int z;
    for (int i = 1; i <= halflen; i++) {
        if (i > r) {
            l = r = i;
            while (r < len && input[lastpos - (r - l)] == input[lastpos - r]) {
                r++;
            }
            z = r - l;
            r--;
        } else {
            int k = i - l;
            if (buf[k] < r - i + 1) {
                // z[k] is less than remaining interval
                z = buf[k];
            } else {
                l = i;
                while (r < len && input[lastpos - (r - l)] == input[lastpos - r]) {
                    r++;
                }
                z = r - l;
                r--;
            }
        }

        if (z >= i) {
            // Done. Found position where the substring at the end of the input sequence is
            // immediately repeated
            return i;
        } else {
            // Store z-value. It can potentially be used to determine later z-values (thereby
            // ensuring the algorithm requires a minimal amount of comparisons).
            buf[i] = z;
        }
    }

    return 0;
}

