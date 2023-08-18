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
    Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
    Ins::TURN, Ins::NOOP,  Ins::UNSET
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
}

bool ExhaustiveSearcher::instructionStackEquals(Ins* reference) const {
    auto p1 = _instructionStack.begin();
    Ins* p2 = reference;

    while (p1 != _instructionStack.end() && *p2 != Ins::UNSET && *p1 == *p2) {
        p1++;
        p2++;
    }

    return (p1 == _instructionStack.end() && *p2 == Ins::UNSET);
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
    << ", TestHangDetection = " << _settings.testHangDetection
    << std::endl;
}

void ExhaustiveSearcher::dump() {
    _program.dump(_pp.p);
}

void ExhaustiveSearcher::setProgressTracker(ProgressTracker* tracker) {
    _tracker = tracker;
}

void ExhaustiveSearcher::verifyHang() {
    RunResult result = _fastExecutor.execute(&_programBuilder);
    switch (result) {
        case RunResult::SUCCESS:
            _tracker->reportDone(_fastExecutor.numSteps());
            break;
        case RunResult::PROGRAM_ERROR:
            _tracker->reportLateEscape(_fastExecutor.numSteps());
            break;
        case RunResult::DATA_ERROR:
            _tracker->reportError();
            break;
        case RunResult::DETECTED_HANG:
        case RunResult::ASSUMED_HANG:
            _tracker->reportAssumedHang();
            break;
        case RunResult::UNKNOWN:
            assert(false);
    }
}


void ExhaustiveSearcher::extendBlock() {
    while (true) {
        InstructionPointer ip;
        Ins ins;
        do {
            ip = nextInstructionPointer(_pp);
            ins = _program.getInstruction(ip);

            switch (ins) {
                case Ins::DONE:
                    _programBuilder.incSteps();
                    _programBuilder.finalizeExitBlock();
                    run();
                    return;
                case Ins::UNSET:
                    branch(); // Extend block by continuing search
                    return;
                case Ins::DATA:
                    _programBuilder.addDataInstruction(_pp.dir);
                    break;
                case Ins::TURN:
                    if (_programBuilder.isInstructionSet()) {
                        const ProgramBlock *block = _programBuilder.finalizeBlock(_pp.p);
                        if (!_settings.disableNoExitHangDetection &&
                            !_exitFinder.canExitFrom(block)
                        ) {
                            _tracker->reportDetectedHang(HangType::NO_EXIT,
                                                         _settings.testHangDetection);
                            if (_settings.testHangDetection) verifyHang();
                        } else {
                            run();
                        }
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
        } while (ins == Ins::TURN);

        _pp.p = ip;
        _programBuilder.incSteps();

        if (_programBuilder.getNumSteps() > 64) {
            _programBuilder.finalizeHangBlock();
            run();
            return;
        }
    }
}

void ExhaustiveSearcher::buildBlock(const ProgramBlock* block) {
    assert(!block->isFinalized());
    TurnDirection td0 = _td;

    _pp = _programBuilder.getStartProgramPointer(block, _program);
    _td = _programBuilder.turnDirectionForBlock(block);

    _programBuilder.enterBlock(block);
    extendBlock();

    _td = td0;
}

void ExhaustiveSearcher::branch() {
    bool resuming = *_resumeFrom != Ins::UNSET;
    bool abortSearch = (_searchMode == SearchMode::FIND_ONE
                        || (_searchMode == SearchMode::SUB_TREE && resuming));
    InstructionPointer ip = nextInstructionPointer(_pp);

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

        _program.setInstruction(ip, ins);
        _instructionStack.push_back(ins);
        _programBuilder.push();
        ProgramPointer pp0 = _pp;

//        if (atTargetProgram()) {
//            _program.dump();
//        }

        extendBlock();

        _program.clearInstruction(ip);
        _instructionStack.pop_back();
        _programBuilder.pop();
        _pp = pp0;

        if (abortSearch) {
            break;
        }
    }
}

void ExhaustiveSearcher::run() {
    if (_programExecutor == &_fastExecutor && *_resumeFrom == Ins::UNSET) {
        // We're done resuming and switching to the hang executor. Pass how many steps of the
        // current program have already been executed by the fast executor. Hang detection can be
        // disabled up till then. We are about to execute a new program block, so up till this
        // point we cannot detect any hangs.
        _programExecutor = &_hangExecutor;
        _hangExecutor.setHangDetectionStart(_fastExecutor.numSteps());
    }

    ProgramExecutor *executor = _programExecutor;

    executor->push();
    switch (executor->execute(&_programBuilder)) {
        case RunResult::SUCCESS: {
            _tracker->reportDone(executor->numSteps());
            break;
        }
        case RunResult::PROGRAM_ERROR: {
            const ProgramBlock* block = executor->lastProgramBlock();

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
            HangType hangType = executor->detectedHangType();
            bool testHang = _settings.testHangDetection && (hangType == HangType::REGULAR_SWEEP ||
                                                            hangType == HangType::IRREGULAR_SWEEP ||
                                                            hangType == HangType::APERIODIC_GLIDER);
            if (hangType == HangType::NO_DATA_LOOP) {
                _tracker->reportDetectedHang(hangType, testHang);
            } else {
                _tracker->reportDetectedHang(_hangExecutor.detectedHang(), testHang);
            }
            if (testHang) {
                verifyHang();
            }
            break;
        }
        case RunResult::UNKNOWN:
            assert(false);
    }
    executor->pop();
}

Ins noResumeStack[] = { Ins::UNSET };
void ExhaustiveSearcher::search() {
    _resumeFrom = noResumeStack;
    _programExecutor = &_hangExecutor;

    // Note: Even though the searcher can carry out multiple searches, there is no need to reset
    // the program executors or program builder before the search. Their state is restored when the
    // search backtracks.

    run();
}

void ExhaustiveSearcher::search(Ins* resumeFrom) {
    _resumeFrom = resumeFrom;
    _programExecutor = &_fastExecutor;

    std::cout << "Resuming from: ";
    ::dumpInstructionStack(_resumeFrom);

    run();
}

void ExhaustiveSearcher::searchSubTree(Ins* resumeFrom) {
    _searchMode = SearchMode::SUB_TREE;
    search(resumeFrom);
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
