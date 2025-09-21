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

Ins validInstructions[] = { Ins::NOOP, Ins::DATA, Ins::TURN };

Ins targetStack[] = {
    Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA,
    Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
    Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
};

ExhaustiveSearcher::ExhaustiveSearcher(int width, int height, SearchSettings settings) :
    _settings(settings),
    _program(width, height),
    _programBuilder(std::make_shared<InterpretedProgramBuilder>()),
    _exitFinder(_program, *_programBuilder),
    _hangExecutor(settings.dataSize, settings.maxHangDetectionSteps),
    _fastExecutor(settings.dataSize)
{
    _fastExecutor.setMaxSteps(_settings.maxSteps);
    _hangExecutor.setMaxSteps(_settings.maxSearchSteps);
    _hangExecutor.addDefaultHangDetectors();

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

void ExhaustiveSearcher::dumpInstructionStack(const std::string& sep) const {
    ::dumpInstructionStack(_instructionStack, sep.size() ? sep : ",");
}

void ExhaustiveSearcher::dumpSettings() {
    std::cout
    << "Size = " << _program.getWidth() << "x" << _program.getHeight()
    << ", DataSize = " << _settings.dataSize
    << ", MaxSteps = " << _settings.maxHangDetectionSteps
      << "/" << _settings.maxSearchSteps
      << "/" << _settings.maxSteps
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
    RunResult result = _fastExecutor.execute(_programBuilder);
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
                    _programBuilder->incSteps();
                    _programBuilder->finalizeExitBlock();
                    run();
                    return;
                case Ins::UNSET:
                    branch(); // Extend block by continuing search
                    return;
                case Ins::DATA:
                    _programBuilder->addDataInstruction(_pp.dir);
                    break;
                case Ins::TURN:
                    if (_programBuilder->isInstructionSet()) {
                        const ProgramBlock *block = _programBuilder->finalizeBlock(_pp.p);
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
        _programBuilder->incSteps();

        if (_programBuilder->getNumSteps() > 64) {
            _programBuilder->finalizeHangBlock();
            run();
            return;
        }
    }
}

void ExhaustiveSearcher::buildBlock(const ProgramBlock* block) {
    assert(!block->isFinalized());
    TurnDirection td0 = _td;

    _pp = _programBuilder->getStartProgramPointer(block, _program);
    _td = _programBuilder->turnDirectionForBlock(block);

    _programBuilder->enterBlock(block);
    extendBlock();

    _td = td0;
}

void ExhaustiveSearcher::branch() {
    bool resuming = _resuming;
    bool abortSearch = (_searchMode == SearchMode::FIND_ONE
                        || (_searchMode == SearchMode::SUB_TREE && resuming));
    InstructionPointer ip = nextInstructionPointer(_pp);

    for (int i = 0; i < 3; i++) {
        Ins ins = validInstructions[i];

        if (resuming) {
            if (ins == *_resumeIns) {
                _resumeIns++;

                resuming = false; // Let search continue in FULL_TREE search mode
            } else {
                continue;
            }
        }

        _program.setInstruction(ip, ins);
        _instructionStack.push_back(ins);
        _programBuilder->push();
        ProgramPointer pp0 = _pp;

//        if (atTargetProgram()) {
//            _program.dump();
//            _hangExecutor.setVerbose(true);
//        } else {
//            _hangExecutor.setVerbose(false);
//        }

        extendBlock();

        _program.clearInstruction(ip);
        _instructionStack.pop_back();
        _programBuilder->pop();
        _pp = pp0;

        if (abortSearch) {
            break;
        }
    }
}

void ExhaustiveSearcher::switchToHangExecutor() {
    // The program should be resuming
    assert(_resuming);

    // The program should have finished resumption (otherwise the search is likely
    // wrongly configured)
    assert(_resumeIns == _resumeEnd);

    // We're done resuming and switching to the hang executor. Pass how many steps of
    // the current program have already been executed by the fast executor. Hang
    // detection can be disabled up till then. We are about to execute a new program
    // block, so up till this point we cannot detect any hangs.
    _hangExecutor.setHangDetectionStart(_fastExecutor.numSteps());
    _hangExecutor.setMaxSteps(_fastExecutor.numSteps() + _settings.maxSearchSteps);
    _programExecutor = &_hangExecutor;
    _resuming = false;

    // TODO: Let hang executor continue from current point
    // Copy data from fast executor and only start data-undo stack from this moment
}

void ExhaustiveSearcher::run() {
    ProgramExecutor *executor = _programExecutor;
    switch (executor->execute(_programBuilder)) {
        case RunResult::SUCCESS: {
            _tracker->reportDone(executor->numSteps());
            break;
        }
        case RunResult::PROGRAM_ERROR: {
            if (executor == &_hangExecutor || _resuming) {
                if (_resuming && _resumeIns == _resumeEnd) {
                    switchToHangExecutor();
                }

                buildBlock(executor->lastProgramBlock());
            } else {
                _tracker->reportLateEscape(executor->numSteps());
            }
            break;
        }
        case RunResult::DATA_ERROR: {
            _tracker->reportError();
            break;
        }
        case RunResult::ASSUMED_HANG: {
            if (_resuming) {
                // The fast executor finished its configured number of steps (after, presumably, it
                // finished resuming). Switch to hang detection.

                switchToHangExecutor();

                run();
            } else if (executor == &_hangExecutor) {
                // Hang detection is finished. Switch to fast execution

                _tracker->reportFastExecution();
                _fastExecutor.setMaxSteps(_settings.maxSteps);
                _fastExecutor.resumeFrom(_hangExecutor.lastProgramBlock(),
                                         _hangExecutor.getData(),
                                         _hangExecutor.numSteps());
                _programExecutor = &_fastExecutor;

                run();

                _programExecutor = &_hangExecutor;
            } else {
                _tracker->reportAssumedHang();
            }

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

std::vector<Ins> emptyStack;
void ExhaustiveSearcher::search() {
    _resumeIns = emptyStack.cbegin();
    _resumeEnd = emptyStack.cend();
    _resuming = false;

    _programExecutor = &_hangExecutor;

    // Note: Even though the searcher can carry out multiple searches, there is no need to reset
    // the program executors or program builder before the search. Their state is restored when the
    // search backtracks.

    run();
}

void ExhaustiveSearcher::search(const std::vector<Ins> &resumeFrom, int fromSteps) {
    _resumeIns = resumeFrom.cbegin();
    _resumeEnd = resumeFrom.cend();
    _resuming = true;
    _fastExecutor.setMaxSteps(fromSteps ? fromSteps : _settings.maxSteps);
    _programExecutor = &_fastExecutor;

    std::cout << "Resuming from: ";
    ::dumpInstructionStack(resumeFrom);

    run();
}

void ExhaustiveSearcher::searchSubTree(const std::vector<Ins> &resumeFrom, int fromSteps) {
    _searchMode = SearchMode::SUB_TREE;
    search(resumeFrom, fromSteps);
    _searchMode = SearchMode::FULL_TREE;
}

void ExhaustiveSearcher::findOne() {
    _searchMode = SearchMode::FIND_ONE;
    search();
    _searchMode = SearchMode::FULL_TREE;
}

void ExhaustiveSearcher::findOne(const std::vector<Ins> &resumeFrom) {
    _searchMode = SearchMode::FIND_ONE;
    search(resumeFrom);
    _searchMode = SearchMode::FULL_TREE;
}
