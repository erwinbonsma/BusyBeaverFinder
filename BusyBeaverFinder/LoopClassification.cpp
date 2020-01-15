//
//  LoopClassification.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "LoopClassification.h"

#include <iostream>

#include "ProgramBlock.h"
#include "RunSummary.h"
#include "InterpretedProgram.h"

void ExitCondition::init(Operator op, int value, int dpOffset) {
    _operator = op;
    _value = value;
    _dpOffset = dpOffset;

    _modulus = 1;
}

bool ExitCondition::isTrueForValue(int value) {
    switch (_operator) {
        case Operator::EQUALS:
            return (value == _value);
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

    return mod1 == mod2 || (mod1 + abs(_modulus)) % _modulus == (mod2 + abs(_modulus)) % _modulus;
}

void ExitCondition::dump(bool bootstrapOnly) {
    std::cout << "Data[" << _dpOffset << "] ";
    switch (_operator) {
        case Operator::EQUALS: std::cout << "=="; break;
        case Operator::LESS_THAN_OR_EQUAL: std::cout << "<="; break;
        case Operator::GREATER_THAN_OR_EQUAL: std::cout << ">="; break;
    }
    std::cout << " " << _value;

    if (abs(_modulus) > 1) {
        std::cout << ", Modulus = " << _modulus;
    }

    if (bootstrapOnly) {
        std::cout << ", Bootstrap only";
    }

    std::cout << std::endl;
}


LoopClassification::LoopClassification() {
    _dpDelta = 0;
    _numDataDeltas = 0;
}

int LoopClassification::deltaAt(int dpOffset) {
    int deltaIndex = 0;

    // Find existing delta record, if any
    while (deltaIndex < _numDataDeltas && _dataDelta[deltaIndex].dpOffset() != dpOffset) {
        deltaIndex++;
    }

    return (deltaIndex == _numDataDeltas) ? 0 : _dataDelta[deltaIndex].delta();
}

int LoopClassification::updateDelta(int dpOffset, int delta) {
    int deltaIndex = 0;

    // Find existing delta record, if any
    while (deltaIndex < _numDataDeltas && _dataDelta[deltaIndex].dpOffset() != dpOffset) {
        deltaIndex++;
    }

    if (deltaIndex == _numDataDeltas) {
        // An existing record was not found,  so create one.
        assert(_numDataDeltas < maxDataDeltasPerLoop);
        _dataDelta[_numDataDeltas++].init(dpOffset);
    }

    if (_dataDelta[deltaIndex].changeDelta(delta)) {
        // This change cancelled out previous changes. Remove the entry to reflect this
        if (deltaIndex != --_numDataDeltas) {
            _dataDelta[deltaIndex] = _dataDelta[_numDataDeltas];
        }

        return 0;
    } else {
        return _dataDelta[deltaIndex].delta();
    }
}

void LoopClassification::squashDeltas() {
    int i = 0;
    while (i < _numDataDeltas) {
        int dpOffsetMod = _dataDelta[i]._dpOffset % _dpDelta;
        if (dpOffsetMod * _dpDelta < 0) {
            // Ensure modulus is canonical and sign matches that of delta DP
            dpOffsetMod += _dpDelta;
        }

        int j = _numDataDeltas;
        while (--j > i) {
            int dpOffsetMod2 = _dataDelta[j]._dpOffset % _dpDelta;
            if (dpOffsetMod2 * _dpDelta < 0) {
                dpOffsetMod2 += _dpDelta;
            }
            if (dpOffsetMod == dpOffsetMod2) {
                // Squash it
                _dataDelta[i]._delta += _dataDelta[j]._delta;

                if (j != --_numDataDeltas) {
                    _dataDelta[j] = _dataDelta[_numDataDeltas];
                }
            }
        }

        if (_dataDelta[i]._delta == 0) {
            // The result is no change. Remove this entry as well.
            if (i != --_numDataDeltas) {
                _dataDelta[i] = _dataDelta[_numDataDeltas];
            }
        } else {
            _dataDelta[i]._dpOffset = dpOffsetMod;
        }

        i++;
    }
}

void LoopClassification::initExitsForStationaryLoop() {
    // Initialise all exits
    for (int i = 0; i < _numBlocks; i++) {
        LoopExit& loopExit = _loopExit[i];
        int dp = _effectiveResult[i].dpOffset();
        int currentDelta = _effectiveResult[i].delta();
        int finalDelta = deltaAt(dp);

        if (finalDelta == 0) {
            loopExit.exitCondition.init(Operator::EQUALS, -currentDelta, dp);
        } else {
            if (finalDelta > 0) {
                loopExit.exitCondition.init(Operator::LESS_THAN_OR_EQUAL, -currentDelta, dp);
            } else {
                loopExit.exitCondition.init(Operator::GREATER_THAN_OR_EQUAL, -currentDelta, dp);
            }
            loopExit.exitCondition.setModulusConstraint(finalDelta);
        }

        // Reset status
        loopExit.bootstrapOnly = false;
    }

    // Identify bootstrap-only exits (and exits that can never be reached).
    for (int i = _numBlocks; --i >= 0; ) {
        LoopExit& loopExit = _loopExit[i];
        int dp = _effectiveResult[i].dpOffset();
        int delta = _effectiveResult[i].delta();
        int mc = loopExit.exitCondition.modulusConstraint();
        int deltaMod = delta % mc;
        if (deltaMod < 0) {
            deltaMod += abs(mc);
        }

        for (int j = i; --j >= 0; ) {
            if (
                j != i &&
                _effectiveResult[j].dpOffset() == dp
            ) {
                int delta2 = _effectiveResult[j].delta();
                int delta2Mod = delta2 % mc;
                if (delta2Mod < 0) {
                    delta2Mod += abs(mc);
                }

                if (delta2Mod == deltaMod) {
                    // One of these instructions cancels the other out. Determine the one

                    if (
                        // In case of equal deltas, j cancels out i, as it executes first
                        delta2 == delta ||
                        // Otherwise, it depends on the size of the delta (wrt to the change dir)
                        (mc > 0 && delta2 > delta) ||
                        (mc < 0 && delta2 < delta)
                    ) {
                        loopExit.bootstrapOnly = true;
                    } else {
                        _loopExit[j].bootstrapOnly = true;
                    }
                }
            }
        }
    }
}

void LoopClassification::initExitsForNonStationaryLoop() {
    // Initialise all exits
    for (int i = _numBlocks; --i >= 0; ) {
        LoopExit& loopExit = _loopExit[i];
        int dp = _effectiveResult[i].dpOffset();
        int currentDelta = _effectiveResult[i].delta();

        loopExit.exitCondition.init(Operator::EQUALS, -currentDelta, dp);
        loopExit.bootstrapOnly = true; // Initial assumption
    }

    // Identify bootstrap-only exits.
    for (int i = abs(_dpDelta); --i >= 0; ) {
        // The index of first instruction that encounters data value at relative position i
        int first = -1;
        // The DP offset of the instruction that encountered the data value first. A later
        // instruction that looks further ahead may beat an earlier instruction
        int firstDpDiv = 0;

        for (int j = _numBlocks; --j >= 0; ) {
            int dp = _effectiveResult[j].dpOffset();
            int dp_rem = dp % _dpDelta;
            if (dp_rem < 0) {
                dp_rem += abs(_dpDelta);
            }

            if (dp_rem == i) {
                // This instruction examines the current value

                int dpDiv = dp / _dpDelta;
                if (first == -1 || dpDiv > firstDpDiv) {
                    // Furthermore, so far it is the first to examine it
                    first = j;
                    firstDpDiv = dpDiv;
                }
            }
        }

        if (first != -1) {
            _loopExit[first].bootstrapOnly = false;
        }
    }
}

void LoopClassification::classifyLoop() {
    _dpDelta = 0;
    _numDataDeltas = 0;

    int minDp = 0;
    int maxDp = 0;
    int i = 0;

    // Determine the intermediate results and final results of a single loop iteration
    ProgramBlock* programBlock = _loopBlocks[0];
    ProgramBlock* prevBlock = _loopBlocks[_numBlocks - 1];
    while (i < _numBlocks) {
        if (programBlock->isDelta()) {
            int effectiveDelta = updateDelta(_dpDelta, programBlock->getInstructionAmount());
            _effectiveResult[i].init(_dpDelta);
            _effectiveResult[i].changeDelta(effectiveDelta);
        } else {
            _dpDelta += programBlock->getInstructionAmount();
            minDp = std::min(minDp, _dpDelta);
            maxDp = std::max(maxDp, _dpDelta);

            _effectiveResult[i].init(_dpDelta);
            _effectiveResult[i].changeDelta(deltaAt(_dpDelta));
        }

        // For now, the classification only supports blocks where each loop-exit is zero-based.
        // Verify that this assumption holds for the current loop.
        assert(prevBlock->nonZeroBlock() == programBlock);

        prevBlock = programBlock++;
        i++;
    }

    // Collapse the results considering multiple loop iterations (only for non-stationary loops)
    if (_dpDelta != 0) {
        squashDeltas();
        initExitsForNonStationaryLoop();

        _numBootstrapCycles = (maxDp - minDp) / abs(_dpDelta);
    } else {
        initExitsForStationaryLoop();

        _numBootstrapCycles = 0;
    }
}

void LoopClassification::classifyLoop(ProgramBlock* entryBlock, int numBlocks) {
    _numBlocks = numBlocks;

    for (int i = _numBlocks; --i >= 0; ) {
        _loopBlocks[i] = entryBlock + i;
    }

    classifyLoop();
}

void LoopClassification::classifyLoop(InterpretedProgram& program,
                                      RunSummary& runSummary,
                                      RunBlock* runBlock) {
    assert(runBlock->isLoop());

    _numBlocks = runBlock->getLoopPeriod();
    for (int i = _numBlocks; --i >= 0; ) {
        int index = runSummary.programBlockIndexAt(runBlock->getStartIndex() + i);

        _loopBlocks[i] = program.getBlock(index);
    }

    classifyLoop();
}


void LoopClassification::dump() {
    std::cout << "delta DP = " << _dpDelta << std::endl;

    for (int i = 0; i < _numDataDeltas; i++) {
        if (i > 0) {
            std::cout << ", ";
        }
        std::cout << "[" << _dataDelta[i].dpOffset() << "]";
        if (_dataDelta[i].delta() > 0) {
            std::cout << "+";
        }
        std::cout << _dataDelta[i].delta();
    }
    std::cout << std::endl;

    for (int i = 0; i < _numBlocks; i++) {
        std::cout << "Exit #" << i << ": ";
        _loopExit[i].exitCondition.dump(_loopExit[i].bootstrapOnly);
    }
}
