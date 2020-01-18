//
//  StaticPeriodicHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticPeriodicHangDetector.h"

StaticPeriodicHangDetector::StaticPeriodicHangDetector(ExhaustiveSearcher& searcher)
    : StaticHangDetector(searcher) {}

Trilian StaticPeriodicHangDetector::exhibitsHangBehaviour(bool loopContinues) {
    return loopContinues ? Trilian::YES : Trilian::NO;
}

bool StaticPeriodicHangDetector::analyseLoop(LoopAnalysis &loop, int &loopStart) {
    RunSummary& runSummary = _searcher.getRunSummary();
    RunBlock* loopRunBlock = runSummary.getLastRunBlock();

    loopStart = loopRunBlock->getStartIndex();
    return loop.analyseLoop(_searcher.getInterpretedProgram(), runSummary, loopRunBlock);
}

bool StaticPeriodicHangDetector::checkAllFreshlyConsumedValuesWillBeZero() {
    Data &data = _searcher.getData();

    for (int i = _loop.loopSize(); --i >= 0; ) {
        if (_loop.exit(i).firstForValue) {
            DataPointer p = data.getDataPointer() + _loop.exit(i).exitCondition.dpOffset();
            int count = 0;

            while (
               (_loop.dataPointerDelta() > 0 && p <= data.getMaxBoundP()) ||
               (_loop.dataPointerDelta() < 0 && p >= data.getMinBoundP())
            ) {
                if (*p != 0) {
                    return false;
                }
                p += _loop.dataPointerDelta();
                count++;
                if (count > 32) {
                    _searcher.getInterpretedProgram().dump();
                    _searcher.dumpHangDetection();
                    _loop.dump();
                    assert(false);
                }
            }
        }
    }

    return true;
}

Trilian StaticPeriodicHangDetector::canProofHang(bool resumed) {
    if (!resumed) {
        // We are in a new loop that requires analysis
        if (!analyseLoop(_loop, _loopStart)) {
            return Trilian::NO; // Analysis failed
        }
    }

    int loopLen = _searcher.getRunSummary().getNumProgramBlocks() - _loopStart;
    if (loopLen <= _loop.loopSize() * _loop.numBootstrapCycles()) {
        // Loop is not yet fully bootstrapped. Too early to tell if the loop is hanging
        return Trilian::MAYBE;
    }

    // The detector should only be invoked at the start of a loop iteration (so that the DP-offsets
    // of the exit conditions are correct)
    assert(loopLen % _loop.loopSize() == 0);

    if (_loop.dataPointerDelta() == 0) {
        for (int i = _loop.loopSize(); --i >=0; ) {
            LoopExit &exit = _loop.exit(i);
            if (exit.exitWindow == ExitWindow::ANYTIME) {
                Data &data = _searcher.getData();
                int value = *(data.getDataPointer() + exit.exitCondition.dpOffset());
                if (exit.exitCondition.isTrueForValue(value)) {
                    return Trilian::NO;
                }
            }
        }
    } else {
        for (int i = _loop.loopSize(); --i >=0; ) {
            LoopExit &exit = _loop.exit(i);
            if (exit.exitWindow == ExitWindow::ANYTIME) {
                if (exit.exitCondition.isTrueForValue(0)) {
                    return Trilian::NO;
                }
            }
        }

        // This loop is only guaranteed to hang when all data values that the loop will freshly
        // consume are zero.
        //
        // Note: A complicating factor is that the loop may already have consumed some data values
        // ahead of its current DP and may still freshly consume some datas behind its DP.
        if (!checkAllFreshlyConsumedValuesWillBeZero()) {
            // We cannot conclude this is a hang.
            return Trilian::MAYBE;
        }
    }

//    _searcher.dumpHangDetection();
//    _searcher.getInterpretedProgram().dump();
//    _loop.dump();

    // None of the exit conditions can be met
    return Trilian::YES;
}
