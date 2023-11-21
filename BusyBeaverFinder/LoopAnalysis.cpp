//
//  LoopAnalysis.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "LoopAnalysis.h"

#include <algorithm>
#include <array>
#include <iostream>

#include "Data.h"
#include "ProgramBlock.h"
#include "RunSummary.h"
#include "InterpretedProgram.h"
#include "Utils.h"

// It is set big enough so that the effective increment realized by a loop is always smaller.
const int UNSET_FIXED_VALUE = 1024;

void ExitCondition::init(Operator op, int value, int dpOffset) {
    _operator = op;
    _value = value;
    _dpOffset = dpOffset;

    _modulus = 1;
}

bool ExitCondition::isTrueForValue(int value) const {
    switch (_operator) {
        case Operator::EQUALS:
            return (value == _value);
        case Operator::UNEQUAL:
            return (value != _value);
        case Operator::LESS_THAN_OR_EQUAL:
            if (value > _value) {
                return false;
            }
            break;
        case Operator::GREATER_THAN_OR_EQUAL:
            if (value < _value) {
                return false;
            }
            break;
    }

    int mod1 = value % _modulus;
    int mod2 = _value % _modulus;

    return mod1 == mod2 || (mod1 + _modulus) % _modulus == (mod2 + _modulus) % _modulus;
}

std::ostream &operator<<(std::ostream &os, const ExitCondition &ec) {
    os << "Data[" << ec.dpOffset() << "] ";
    switch (ec.getOperator()) {
        case Operator::EQUALS: os << "=="; break;
        case Operator::UNEQUAL: os << "!="; break;
        case Operator::LESS_THAN_OR_EQUAL: os << "<="; break;
        case Operator::GREATER_THAN_OR_EQUAL: os << ">="; break;
    }
    os << " " << ec.value();

    if (ec.modulusConstraint() > 1) {
        os << ", Modulus = " << ec.modulusConstraint();
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const LoopExit &le) {
    os << le.exitCondition;

    switch (le.exitWindow) {
        case ExitWindow::BOOTSTRAP: os << ", Bootstrap only"; break;
        case ExitWindow::NEVER: os << ", Unreachable"; break;
        default: ; // void
    }

    if (le.firstForValue) {
        os << ", First consumer";
    }

    return os;
}

LoopAnalysis::LoopAnalysis() : SequenceAnalysis() {
}

bool LoopAnalysis::exitsOnZero(int index) {
    const ProgramBlock* curBlock = _programBlocks[index];
    const ProgramBlock* nxtBlock = _programBlocks[(index + 1) % loopSize()];

    return curBlock->nonZeroBlock() == nxtBlock;
}

int LoopAnalysis::deltaAt(int dpOffset) const {
    int delta = 0;
    int mod = normalizedMod(dpOffset, _dpDelta);

    for (auto &dd : _dataDeltas) {
        if (
            dd.dpOffset() == dpOffset ||
            (
                mod == normalizedMod(dd.dpOffset(), _dpDelta) &&
                sign(dpOffset - dd.dpOffset()) == sign(_dpDelta)
            )
        ) {
            delta += dd.delta();
        }
    }

    return delta;
}

int LoopAnalysis::deltaAtOnNonStandardEntry(int dpOffset, int startingInstruction) const {
    assert(startingInstruction > 0);

    // Establish delta, compensating for DP offset
    auto &ddEntry = effectiveResultAt(startingInstruction - 1);
    int delta = deltaAt(dpOffset + ddEntry.dpOffset());

    // Undo data value changes caused by skipped instructions.
    for (int i = startingInstruction; --i >= 0; ) {
        auto &dd = effectiveResultAt(i);
        if (dd.dpOffset() == (dpOffset + ddEntry.dpOffset())) {
            delta -= dd.delta();
            break;
        }
    }

    return delta;
}

void LoopAnalysis::squashDeltas() {
    _squashedDeltas.clear();

    for (const DataDelta& dd : _dataDeltas) {
        int dpOffsetMod = dd.dpOffset() % _dpDelta;
        if (dpOffsetMod * _dpDelta < 0) {
            // Ensure modulus is canonical and sign matches that of delta DP
            dpOffsetMod += _dpDelta;
        }
        _squashedDeltas.updateDelta(dpOffsetMod, dd.delta());
    }
}

void LoopAnalysis::setExitConditionsForStationaryLoop() {
    _numBootstrapCycles = 0; // Initialize as zero. It is increased as needed.

    _loopExits.clear();
    for (int i = 0; i < loopSize(); i++) {
        _loopExits.emplace_back();
        LoopExit& loopExit = _loopExits.back();
        int dp = _effectiveResult[i].dpOffset();
        int currentDelta = _effectiveResult[i].delta();
        int finalDelta = _dataDeltas.deltaAt(dp);

        if (finalDelta == 0) {
            Operator  op = exitsOnZero(i) ? Operator::EQUALS : Operator::UNEQUAL;
            loopExit.exitCondition.init(op, -currentDelta, dp);
            loopExit.exitWindow = ExitWindow::BOOTSTRAP;
            _numBootstrapCycles = 1;
        } else {
            assert(exitsOnZero(i)); // Otherwise the loop cannot loop
            Operator  op =
                (finalDelta > 0) ? Operator::LESS_THAN_OR_EQUAL : Operator::GREATER_THAN_OR_EQUAL;
            loopExit.exitCondition.init(op, -currentDelta, dp);
            loopExit.exitCondition.setModulusConstraint(abs(finalDelta));

            // Reset to known state. May still be changed later
            loopExit.exitWindow = ExitWindow::ANYTIME;
        }
    }
}

// Identify bootstrap-only exits (and exits that can never be reached).
void LoopAnalysis::identifyBootstrapOnlyExitsForStationaryLoop() {
    for (int i = loopSize(); --i >= 0; ) {
        LoopExit& loopExit = _loopExits[i];

        if (loopExit.exitWindow != ExitWindow::ANYTIME) {
            // This cannot cancel out other exits which are not yet marked as bootstrap-only.
            continue;
        }

        int dp = _effectiveResult[i].dpOffset();
        int delta = _effectiveResult[i].delta();
        int mc = loopExit.exitCondition.modulusConstraint();
        int deltaMod = normalizedMod(delta, mc);

        for (int j = i; --j >= 0; ) {
            if (_effectiveResult[j].dpOffset() == dp) {
                int delta2 = _effectiveResult[j].delta();
                int delta2Mod = normalizedMod(delta2, mc);

                if (delta2Mod == deltaMod) {
                    // One of these instructions (partially) masks the other

                    if (delta != delta2) {
                        // The masked condition can only occur during bootstrap
                        bool deltaIsPositive = _dataDeltas.deltaAt(dp) > 0;
                        int k = (deltaIsPositive == (delta2 > delta)) ? i : j;

                        int numBootstrapCycles = abs((delta2 - delta) / mc);
                        _numBootstrapCycles = std::max(_numBootstrapCycles, numBootstrapCycles);

                        _loopExits[k].exitWindow = ExitWindow::BOOTSTRAP;
                        if (numBootstrapCycles == 1) {
                            // Simplify operator
                            _loopExits[k].exitCondition.setOperator(Operator::EQUALS);
                        }
                    } else {
                        // The masked condition can never occur
                        loopExit.exitWindow = ExitWindow::NEVER;
                    }
                }
            }
        }
    }

    // Final bookkeeping
    for (auto &loopExit : _loopExits) {
        loopExit.firstForValue = (loopExit.exitWindow == ExitWindow::ANYTIME);
    }
}

void LoopAnalysis::markUnreachableExitsForStationaryLoop() {
    for (int i = loopSize(); --i >= 0; ) {
        if (!exitsOnZero(i)) {
            // After this instruction executed successfully and the loop continues the value will
            // be zero. This makes it possible to verify if the instructions that follow it that
            // depends on the same data value can abort the loop.

            int dpOffset = _effectiveResult[i].dpOffset();

            for (int j = i + 1; j < loopSize(); ++j) {
                if (
                    dpOffset == _effectiveResult[j].dpOffset() &&
                    !_loopExits[j].exitCondition.isTrueForValue(
                        _effectiveResult[j].delta() - _effectiveResult[i].delta()
                    )
                ) {
                    _loopExits[j].exitWindow = ExitWindow::NEVER;
                }
            }
        }
    }
}

void LoopAnalysis::initExitsForStationaryLoop() {
    setExitConditionsForStationaryLoop();
    identifyBootstrapOnlyExitsForStationaryLoop();
    markUnreachableExitsForStationaryLoop();
}

void LoopAnalysis::initExitsForTravellingLoop() {
    // Temporary helper array that contains instruction indices, which will be sorted based on
    // the order in which they consume data values.
    static std::array<int, maxLoopSize> indices;

    // Temporary helper array that maintains the delta of the value after an instruction is
    // executed wrt to when the value was first encountered by the loop.
    static std::array<int, maxLoopSize> cumDelta;

    // Temporary helper array that tracks if the instruction has a zero-based continuation
    // condition, thereby fixing the entry value required for the loop to spin up.
    static std::array<int, maxLoopSize> fixedExitValue;

    for (int i = loopSize(); --i >= 0; ) {
        indices[i] = i;
    }

    // Sort instructions by the order in which they inspect new data values
    std::function<bool (const int, const int)> compareUp = [this](const int a, const int b) {
        int diff = _effectiveResult[a].dpOffset() - _effectiveResult[b].dpOffset();
        return diff == 0 ? (a < b) : (diff > 0);
    };
    std::function<bool (const int, const int)> compareDn = [this](const int a, const int b) {
        int diff = _effectiveResult[a].dpOffset() - _effectiveResult[b].dpOffset();
        return diff == 0 ? (a < b) : (diff < 0);
    };

    std::sort(indices.begin(), indices.begin() + loopSize(), _dpDelta > 0 ? compareUp : compareDn);

    _numBootstrapCycles = 0; // Initialize as zero. It is increased as needed.
    _loopExits.clear();
    for (int i = loopSize(); --i >= 0; ) {
        _loopExits.emplace_back();
    }

    // Establish the exit condition for each instruction
    for (int ii = 0 ; ii < loopSize(); ii++ ) {
        int i = indices[ii];
        const ProgramBlock* pb = _programBlocks[i];

        int dp_i = _effectiveResult[i].dpOffset();
        int mod_i = normalizedMod(dp_i, _dpDelta);

        bool hasZeroExit = exitsOnZero(i);
        LoopExit& loopExit = _loopExits[i];
        Operator op = hasZeroExit ? Operator::EQUALS : Operator::UNEQUAL;
        loopExit.exitWindow = ExitWindow::ANYTIME; // Initial assumption

        fixedExitValue[i] = hasZeroExit ? UNSET_FIXED_VALUE : 0;
        cumDelta[i] = pb->isDelta() ? pb->getInstructionAmount() : 0;

        bool foundPreceding = false;
        for (int jj = ii; --jj >= 0; ) {
            int j = indices[jj];
            int dp_j = _effectiveResult[j].dpOffset();
            int mod_j = normalizedMod(dp_j, _dpDelta);

            if (mod_i == mod_j) {
                // Both instructions process the same data values

                if (!foundPreceding) {
                    // Found the instruction preceding the current one
                    foundPreceding = true;

                    // Update the cummulative delta
                    cumDelta[i] += cumDelta[j];

                    loopExit.firstForValue = false;
                    loopExit.exitCondition.init(op, -cumDelta[i], dp_i);

                    // If the value is fixed, propagate its setting
                    if (fixedExitValue[j] != UNSET_FIXED_VALUE) {
                        fixedExitValue[i] = fixedExitValue[j];

                        if (pb->isDelta()) {
                            fixedExitValue[i] += pb->getInstructionAmount();
                        }
                    }
                }

                if (
                    // Both instructions exit on the same data value (wrt to loop-entry)
                    cumDelta[i] == cumDelta[j] ||

                    // The earlier instruction fixes the entry value to a value that does not
                    // trigger the later instruction to exit
                    (
                        fixedExitValue[j] != UNSET_FIXED_VALUE && !exitsOnZero(j) &&
                        !loopExit.exitCondition.isTrueForValue(fixedExitValue[i])
                    )
                ) {
                    if (_effectiveResult[i].dpOffset() == _effectiveResult[j].dpOffset()) {
                        // Both instructions see the same value in the same loop iteration, so the
                        // later instruction will never exit
                        loopExit.exitWindow = ExitWindow::NEVER;

                        break;
                    } else if (loopExit.exitWindow != ExitWindow::BOOTSTRAP) {
                        // The later instruction can still exit the loop during bootstrap
                        loopExit.exitWindow = ExitWindow::BOOTSTRAP;
                        int numBootstrapCycles = abs((dp_j - dp_i) / _dpDelta);
                        _numBootstrapCycles = std::max(_numBootstrapCycles, numBootstrapCycles);
                    }
                }
            }
        }

        if (!foundPreceding) {
            loopExit.firstForValue = true;
            loopExit.exitCondition.init(op, -cumDelta[i], dp_i);
        }
    }
}

bool LoopAnalysis::startAnalysis() {
    return SequenceAnalysis::startAnalysis() && _numProgramBlocks <= maxLoopSize;
}

bool LoopAnalysis::finishAnalysis() {
    SequenceAnalysis::finishAnalysis();

    // Collapse the results considering multiple loop iterations (only for non-stationary loops)
    if (_dpDelta != 0) {
        squashDeltas();
        initExitsForTravellingLoop();
    } else {
        initExitsForStationaryLoop();
    }

    return true;
}

bool LoopAnalysis::analyzeLoop(RawProgramBlocks programBlocks, int len) {
    return analyzeSequence(programBlocks, len);
}

bool LoopAnalysis::allValuesToBeConsumedAreZero(const Data &data) const {
    assert(dataPointerDelta() != 0);

    for (auto &loopExit : _loopExits) {
        if (loopExit.firstForValue) {
            DataPointer p = data.getDataPointer() + loopExit.exitCondition.dpOffset();
            int count = 0;

            while (
               (dataPointerDelta() > 0 && p <= data.getMaxBoundP()) ||
               (dataPointerDelta() < 0 && p >= data.getMinBoundP())
            ) {
                if (*p != 0) {
                    return false;
                }
                p += dataPointerDelta();
                count++;
                if (count > 32) {
                    // Abort. This is not expected to occur in practise for (small) programs that
                    // are actually locked in a periodic hang.
                    return false;
                }
            }
        }
    }

    return true;
}

void LoopAnalysis::dump() const {
    std::cout << *this << std::endl;
}

std::ostream &operator<<(std::ostream &os, const LoopAnalysis &la) {
    os << (const SequenceAnalysis&)la << std::endl;

    for (int i = 0; i < la.sequenceSize(); i++) {
        os << "Instruction #" << i;
        os << ", Exit: " << la.exit(i) << std::endl;
    }

    return os;
}
