//
//  Utils.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "Utils.h"

#include <assert.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

bool enableDebugOutput = false;


const int dx[4] = { 0, 1, 0, -1 };
const int dy[4] = { 1, 0, -1, 0 };

bool isPowerOfTwo(int val) {
    return (val & (val - 1)) == 0;
}

int makePowerOfTwo(int val) {
    int shifts = 0;
    while (val != 0) {
        shifts ++;
        val >>= 1;
    }
    return 1 << shifts;
}

int gcd(int a, int b) {
    assert(a > 0 && b > 0);
    while (a != b) {
        if (a > b) {
            a = a - b;
        } else {
            b = b - a;
        }
    }
    return a;
}

int lcm(int a, int b) {
    return a / gcd(a, b) * b;
}

int floordiv(int a, int b) {
    int result = a / b;
    // For negative results, that were truncated towards zero, compensate by subtracting one
    result -= (result < 0) && (a % b) != 0;
    return result;
}

int ceildiv(int a, int b) {
    int result = a / b;
    // For positive results, that were truncated towards zero, compensate by adding one
    result += (result >= 0) && (a % b) != 0;
    return result;
}

InstructionPointer nextInstructionPointer(ProgramPointer pp) {
    return InstructionPointer {
        .col = (int8_t)(pp.p.col + dx[(int)pp.dir]),
        .row = (int8_t)(pp.p.row + dy[(int)pp.dir])
    };
}

// Not used anymore, but kept for reference. It helps understanding the findPeriod implementation
// and has its own unit tests, which is possibly useful in troubleshooting.
void calculateZArray(const char* input, int* output, int len) {
    int l, r;
    l = r = 0;

    output[0] = 0; // Should not be used
    for (int i = 1; i < len; i++) {
        if (i > r) {
            l = r = i;
            while (r < len && input[r - l] == input[r]) {
                r++;
            }
            output[i] = r - l;
            r--;
        } else {
            int k = i - l;
            if (output[k] < r - i + 1) {
                // z[k] is less than remaining interval
                output[i] = output[k];
            } else {
                l = i;
                while (r < len && input[r - l] == input[r]) {
                    r++;
                }
                output[i] = r - l;
                r--;
            }
        }
    }
}

int findPeriod(const char* input, int* buf, int len) {
    int l, r;
    l = r = 0;

    buf[0] = 0; // Should not be used
    int z;
    for (int i = 1; i < len; i++) {
        if (i > r) {
            l = r = i;
            while (r < len && input[r - l] == input[r]) {
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
                while (r < len && input[r - l] == input[r]) {
                    r++;
                }
                z = r - l;
                r--;
            }
        }

        if (z + i == len) {
            // Done. Found position from where remainder of sequence matches start of sequence
            return i;
        } else {
            // Store z-value. It can potentially be used to determine later z-values (thereby
            // ensuring the algorithm requires a minimal amount of comparisons).
            buf[i] = z;
        }
    }

    return len;
}

std::set<int> deltasCanSumToSet1, deltasCanSumToSet2, deltasCanSumToSet3;
std::vector<int> deltasCanSumToVector1, deltasCanSumToVector2;
bool deltasCanSumTo(std::set<int> deltas, int target) {
    assert(target != 0);

    // Handle trivial cases
    if (deltas.size() == 0) {
        return false;
    }

    // Scan deltas
    auto &forwardDeltas = deltasCanSumToSet1;
    auto &backwardDeltas = deltasCanSumToSet2;
    forwardDeltas.clear();
    backwardDeltas.clear();

    for (int delta : deltas) {
        if (sign(delta) == sign(target)) {
            if (target % delta == 0) {
                return true;
            }
            forwardDeltas.insert(abs(delta));
        } else {
            backwardDeltas.insert(-abs(delta));
        }
    }
    target = abs(target);

    // Try to reach target value
    auto &values = deltasCanSumToSet3;
    auto *curValues = &deltasCanSumToVector1;
    auto *nxtValues = &deltasCanSumToVector2;
    values.clear();
    values.insert(0);
    curValues->clear();
    curValues->push_back(0);

    while (curValues->size() > 0) {
        nxtValues->clear();
        for (int value : *curValues) {
            for (int delta : (value < target) ? forwardDeltas : backwardDeltas) {
                int newValue = value + delta;
                if (
                    newValue == target || (target - value) % delta == 0
                ) {
                    return true;
                }
                if (values.find(newValue) == values.end()) {
                    values.insert(newValue);
                    nxtValues->push_back(newValue);
                }
            }
        }

        auto tmp = curValues;
        curValues = nxtValues;
        nxtValues = tmp;
    }

    return false;
}

void loadResumeStackFromStream(std::istream &input, std::vector<Ins> &resumeStack) {
    int intVal;

    resumeStack.clear();
    while (input >> intVal) {
        resumeStack.push_back((Ins)intVal);
    }
}

// Loads a resume stack from file. It expects the stack to be on a single line, but will skip
// comment lines.
bool loadResumeStackFromFile(std::string inputFile, std::vector<Ins> &resumeStack) {
    std::cout << "Resuming from " << inputFile << std::endl;

    std::ifstream input(inputFile);
    if (!input) {
        std::cout << "Could not read file" << std::endl;
        return false;
    }

    std::string line;
    while (resumeStack.empty() && getline(input, line)) {
        std::replace(line.begin(), line.end(), ',', ' ');

        std::istringstream iss(line);
        loadResumeStackFromStream(iss, resumeStack);
    }

    return !resumeStack.empty();
}

void dumpInstructionStack(const std::vector<Ins> &stack, std::ostream &os, const std::string& sep) {
    int count = 0;
    for (const Ins& ins : stack) {
        if (count++) {
            os << sep;
        }
        os << (int)ins;
    }
}

void dumpInstructionStack(const std::vector<Ins> &stack, const std::string& sep) {
    dumpInstructionStack(stack, std::cout, sep);
    std::cout << std::endl;
}

LogHistogram::LogHistogram(int ini_log_scale, int bins_per_log_scale)
: _bins_per_log_scale(bins_per_log_scale), _ini_log_scale(ini_log_scale) {
    _histogram.emplace_back(getBinUpperBound(0), 0);
}

int LogHistogram::getBinUpperBound(int bin_index) {
    return static_cast<int>(
        round(std::pow(10.0f,
                       static_cast<float>(bin_index + _ini_log_scale) / _bins_per_log_scale))
    );
}

void LogHistogram::add(int value) {
    for (auto& entry : _histogram) {
        if (value <= entry.first) {
            // Found the right bin; bump its count
            entry.second++;
            return;
        }
    }

    // Create one or more new bins
    int upperBound;
    do {
        upperBound = getBinUpperBound(static_cast<int>(_histogram.size()));
        _histogram.emplace_back(upperBound, 0);
    } while (upperBound < value);

    // Add entry to last bin
    _histogram.back().second++;
}

std::ostream &operator<<(std::ostream &os, const LogHistogram &h) {
    int lower = 1;
    for (auto& entry : h._histogram) {
        if (lower > 1) {
            os << ", ";
        }
        os << lower << "-" << entry.first << ":" << entry.second;
        lower = entry.first + 1;
    }

    return os;
}
