//
//  ExhaustiveSearcher.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "ExhaustiveSearcher.h"

#include <iostream>
#include <assert.h>

#include "Utils.h"

#include "GliderHangDetector.h"
#include "MetaPeriodicHangDetector.h"
#include "PeriodicSweepHangDetector.h"
#include "IrregularSweepHangDetector.h"

Ins validInstructions[] = { Ins::NOOP, Ins::DATA, Ins::TURN };

Ins targetStack[] = {
    // 5x5 InfSeqExtendingBothWays2
    Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA,
    Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::UNSET
};

ExhaustiveSearcher::ExhaustiveSearcher(int width, int height, SearchSettings settings) :
    _settings(settings),
    _program(width, height),
    _exitFinder(_program, _programBuilder),
    _hangExecutor(settings.dataSize, settings.maxHangDetectionSteps),
    _fastExecutor(settings.dataSize)
{
    _fastExecutor.setMaxSteps(_settings.maxSteps);
    _hangExecutor.setMaxSteps(_settings.maxSteps);

    // Set default search mode
    _searchMode = SearchMode::FULL_TREE;
    _delayHangDetection = false;
}

bool ExhaustiveSearcher::instructionStackEquals(Ins* reference) const {
    auto p1 = _instructionStack.begin();
    Ins* p2 = reference;

    while (p1 != _instructionStack.end() && *p2 != Ins::UNSET && *p1 == *p2) {
        p1++;
        p2++;
    }

    return (*p1 == *p2);
}

bool ExhaustiveSearcher::atTargetProgram() {
    return instructionStackEquals(targetStack);
}

void ExhaustiveSearcher::dumpInstructionStack() const {
    ::dumpInstructionStack(_instructionStack);
}

void ExhaustiveSearcher::dumpSettings() {
    std::cout
    << "Size = " << _program.getWidth() << "x" << _program.getHeight()
    << ", DataSize = " << _settings.dataSize
    << ", MaxSteps = " << _settings.maxHangDetectionSteps << "/" << _settings.maxSteps
    << ", UndoCapacity = " << _settings.undoCapacity
    << ", TestHangDetection = " << _settings.testHangDetection
    << std::endl;
}

void ExhaustiveSearcher::dump() {
    _program.dump(_pp.p);
}

void ExhaustiveSearcher::setProgressTracker(ProgressTracker* tracker) {
    _tracker = tracker;
}

void ExhaustiveSearcher::extendBlock() {
    while (true) {
        InstructionPointer insP = nextInstructionPointer(_pp);
        Ins ins = _program.getInstruction(insP);

        switch (ins) {
            case Ins::DONE:
                _programBuilder.finalizeExitBlock();
                run();
                return;
            case Ins::UNSET:
                branch();
                return;
            case Ins::DATA:
                _programBuilder.addDataInstruction(_pp.dir);
                break;
            case Ins::TURN:
                if (_programBuilder.isInstructionSet()) {
                    _programBuilder.finalizeBlock(insP);
                    run();
                    return;
                } else {
                    if (_td == TurnDirection::COUNTERCLOCKWISE) {
                        _pp.dir = (Dir)(((int)_pp.dir + 3) % 4);
                    } else {
                        _pp.dir = (Dir)(((int)_pp.dir + 1) % 4);
                    }
                }
                break;
            case Ins::NOOP:
                break;
        }
    }
}

void ExhaustiveSearcher::buildBlock(const ProgramBlock* block) {
    assert(!block->isFinalized());

    _pp = _programBuilder.getStartProgramPointer(block, _program);
    _td = _programBuilder.turnDirectionForBlock(block);

    _programBuilder.enterBlock(block);
    extendBlock();
}

void ExhaustiveSearcher::branch() {
    bool resuming = *_resumeFrom != Ins::UNSET;
    bool abortSearch = (_searchMode == SearchMode::FIND_ONE
                        || (_searchMode == SearchMode::SUB_TREE && resuming));

    for (int i = 0; i < 3; i++) {
        Ins ins = validInstructions[i];

        if (resuming) {
            if (ins == *_resumeFrom) {
                _resumeFrom++;

                resuming = false; // Let search continue in FULL_TREE search mode
            } else {
                continue;
            }
        }

        _program.setInstruction(_pp.p, ins);
        _instructionStack.push_back(ins);
        _programBuilder.push();

        extendBlock();

        _program.clearInstruction(_pp.p);
        _instructionStack.pop_back();
        _programBuilder.pop();

        if (abortSearch) {
            break;
        }
    }
}

void ExhaustiveSearcher::run() {
    RunResult result = (_delayHangDetection
    ? _fastExecutor.execute(&_programBuilder)
    : _hangExecutor.execute(&_programBuilder));

    switch (result) {
        case RunResult::SUCCESS: {
            _tracker->reportDone(_hangExecutor.numSteps());
            return;
        }
        case RunResult::PROGRAM_ERROR: {
            const ProgramBlock* block = (_delayHangDetection
                                         ? _fastExecutor.lastProgramBlock()
                                         : _hangExecutor.lastProgramBlock());

            buildBlock(block);
            break;
        }
        case RunResult::DATA_ERROR: {
            _tracker->reportError();
            break;
        }
        case RunResult::ASSUMED_HANG: {
            _tracker->reportAssumedHang();
            break;
        }
        case RunResult::DETECTED_HANG: {
            HangType hangType = _hangExecutor.detectedHangType();
            bool testHang = _settings.testHangDetection && (hangType == HangType::REGULAR_SWEEP ||
                                                            hangType == HangType::IRREGULAR_SWEEP ||
                                                            hangType == HangType::APERIODIC_GLIDER);
            _tracker->reportDetectedHang(hangType, testHang);
            if (testHang) {
                // TODO: fastExecution();
            }
            break;
        }
        case RunResult::UNKNOWN:
            assert(false);
    }
}

void ExhaustiveSearcher::initSearch() {
    _pp = _program.getStartProgramPointer();
//    _hangDetectionEnd = _settings.maxHangDetectionSteps;
}


Ins noResumeStack[] = { Ins::UNSET };
void ExhaustiveSearcher::search() {
    _resumeFrom = noResumeStack;
    initSearch();

    run();
}

void ExhaustiveSearcher::search(Ins* resumeFrom) {
    _resumeFrom = resumeFrom;
    initSearch();

    std::cout << "Resuming from: ";
    ::dumpInstructionStack(_resumeFrom);

    run();
}

void ExhaustiveSearcher::searchSubTree(Ins* resumeFrom, bool delayHangDetection) {
    _searchMode = SearchMode::SUB_TREE;
    _delayHangDetection = delayHangDetection;
    search(resumeFrom);
    _delayHangDetection = false;
    _searchMode = SearchMode::FULL_TREE;
}

void ExhaustiveSearcher::findOne() {
    _searchMode = SearchMode::FIND_ONE;
    search();
    _searchMode = SearchMode::FULL_TREE;
}

void ExhaustiveSearcher::findOne(Ins* resumeFrom) {
    _searchMode = SearchMode::FIND_ONE;
    search(resumeFrom);
    _searchMode = SearchMode::FULL_TREE;
}
