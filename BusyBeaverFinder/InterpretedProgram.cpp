//
//  InterpretedProgram.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 25/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "InterpretedProgram.h"

void InterpretedProgram::dumpBlock(const ProgramBlock* block, std::ostream &os) const {
    os << indexOf(block) << " (" << block->getStartIndex() << "): ";
    os << *block;

    if (!block->isFinalized()) {
        return;
    }

    os << " => ";

    if (block->zeroBlock() != nullptr) {
        os << indexOf(block->zeroBlock());
    } else {
        os << "-";
    }

    if (block->nonZeroBlock() != nullptr) {
        os << "/" << indexOf(block->nonZeroBlock());
    } else {
        os << "/-";
    }

    os << ", #Steps = " << block->getNumSteps();

    os << ", Entries[";
    for (int i = 0; i < block->numEntryBlocks(); i++) {
        if (i != 0) {
            os << ", ";
        }
        os << indexOf(block->entryBlock(i));
    }
    os << "]";
}

void InterpretedProgram::dump() const {
    size_t numBlocks = numProgramBlocks();
    std::cout << "numBlocks = " << numBlocks << std::endl;
    for (int i = 0; i < numBlocks; ++i) {
        dumpBlock(programBlockAt(i), std::cout);

        std::cout << std::endl;
    }
}

char InterpretedProgram::charForIndex(int index) const {
    assert(index >= 0);
    if (index < 26) {
        return 'a' + index;
    }
    index -= 26;
    if (index < 26) {
        return 'A' + index;
    }
    return '?';
}

int InterpretedProgram::indexForChar(char ch) const {
    if (ch >= 'a' && ch <= 'z') {
        return ch - 'a';
    }
    if (ch >= 'A' && ch <= 'Z') {
        return (ch - 'A') + 26;
    }
    assert(false);
}

void InterpretedProgram::dumpShortProgram(std::ostream &os) const {
    for (int i = 0; i < numProgramBlocks(); ++i) {
        const ProgramBlock* block = programBlockAt(i);
        if (i != 0) {
            os << " ";
        }
        os << charForIndex(indexOf(block));

        if (!block->isFinalized()) {
            os << "?";
            continue;
        }
        if (block->isExit()) {
            os << "X";
            continue;
        }
        if (block->isHang()) {
            os << "H";
            continue;
        }

        os << (block->isDelta()
               ? (block->getInstructionAmount() > 0 ? "+" : "-")
               : (block->getInstructionAmount() > 0 ? ">" : "<"));
        os << abs(block->getInstructionAmount());

        auto charForBlockFn = [this](const ProgramBlock* block) {
            return block ? charForIndex(indexOf(block)) : '-';
        };

        os << charForBlockFn(block->zeroBlock());
        os << charForBlockFn(block->nonZeroBlock());
    }
}

void InterpretedProgram::dumpBlockSizes(std::ostream &os) const {
    for (int i = 0; i < numProgramBlocks(); ++i) {
        const ProgramBlock* block = programBlockAt(i);
        if (i != 0) {
            os << " ";
        }
        os << block->getNumSteps();
    }
}

std::string InterpretedProgram::shortProgramString() const {
    std::ostringstream os;
    dumpShortProgram(os);
    return os.str();
}

std::string InterpretedProgram::blockSizeString() const {
    std::ostringstream os;
    dumpBlockSizes(os);
    return os.str();
}

std::deque<int> InterpretedProgramFromString::sizesFromString(std::string& sizesString) {
    std::istringstream stream{sizesString};
    std::deque<int> sizes;

    int size;
    while (stream >> size) {
        sizes.push_back(size);
    }
    return sizes;
}

ProgramBlock* InterpretedProgramFromString::blockForChar(char ch) {
    if (ch == '-') {
        return nullptr;
    }
    int index = indexForChar(ch);
    assert(index < _blocks.size());
    return &_blocks[index];
}

void InterpretedProgramFromString::finalizeBlock(std::string& blockSpec, int numSteps) {
    assert(blockSpec.size() >= 2);
    int index = indexForChar(blockSpec[0]);
    assert(index < _blocks.size());
    ProgramBlock& block = _blocks[index];
    assert(!block.isFinalized());

    char op = blockSpec[1];
    if (op == '?') {
        return;
    }
    if (op == 'X') {
        block.finalizeExit(numSteps);
        return;
    }
    if (op == 'H') {
        block.finalizeHang();
        return;
    }
    assert(op == '<' || op == '>' || op == '+' || op =='-');
    assert(blockSpec.size() == 5);
    bool isDelta = (op == '+' || op == '-');
    bool isNegative = (op == '-' || op == '<');
    int amount = blockSpec[2] - '0';

    block.finalize(isDelta, amount * (isNegative ? -1 : 1), numSteps,
                   blockForChar(blockSpec[3]), blockForChar(blockSpec[4]));
}

InterpretedProgramFromString::InterpretedProgramFromString(std::string& programString,
                                                           std::string& sizesString) {
    std::deque<int> sizes = sizesFromString(sizesString);
    for (int i = 0; i < sizes.size(); ++i) {
        _blocks.emplace_back(i);
    }

    std::istringstream stream{programString};
    std::string blockString;
    while (stream >> blockString) {
        assert(!sizes.empty());
        finalizeBlock(blockString, sizes.front());
        sizes.pop_front();
    }
}
