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

// Global flag that can be used for debugging. Intended usage: when a strange/anomalous situation
// is encountered during a search in a low-level component, it can set this flag. A higher-level
// object (e.g. the searcher) can monitor this and output information about the search state so
// that hopefully the behavior can be isolated/captured in a small unit test.
extern bool enableDebugOutput;

#define PROGRAM_POINTERS_MATCH(pp1, pp2) ( \
    pp1.p.col == pp2.p.col && \
    pp1.p.row == pp2.p.row && \
    pp1.dir == pp2.dir \
)

// Least common multiple
int lcm(int a, int b);

int floordiv(int a, int b);
int ceildiv(int a, int b);

bool isPowerOfTwo(int val);
int makePowerOfTwo(int val);

// Returns a value that is always possible, i.e. 0 <= result < abs(modulus)
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

void dumpInstructionStack(const std::vector<Ins> &stack, const std::string& sep = ",");
void dumpInstructionStack(const std::vector<Ins> &stack, std::ostream &os,
                          const std::string& sep = ",");

// A histogram with logarithmic scaling of the bin sizes.
//
// The number of bins grow as needed.
class LogHistogram {
    friend std::ostream &operator<<(std::ostream &os, const LogHistogram &h);

    std::vector<std::pair<int,long>> _histogram;
    int _bins_per_log_scale;
    int _ini_log_scale;

    int getBinUpperBound(int bin_index);

public:
    // The parameter ini_log_scale (n) determines the first log scale. Its upper bound is 10^n.
    // Each log scale can be sub-divided in more than one bin using "bins_per_log_scale".
    LogHistogram(int ini_log_scale = 1, int bins_per_log_scale = 1);

    // Adds the value to the corresponding bin.
    void add(int value);
};

std::ostream &operator<<(std::ostream &os, const LogHistogram &h);

template<typename T, std::size_t N, std::size_t... I>
constexpr auto create_indexed_array_impl(std::index_sequence<I...>) {
    return std::array<T, N>{ {I...} };
}

// Function that creates an array where the element at index I of type T is constructed using T(I)
template<typename T, std::size_t N>
constexpr auto create_indexed_array() {
    return create_indexed_array_impl<T, N>(std::make_index_sequence<N>{});
}

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

