//
//  RunSummary.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 07/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "RunSummary.h"

#include <iostream>
#include <vector>

#include "ProgramBlock.h"
#include "InterpretedProgram.h"
#include "Utils.h"

void RunSummaryBase::reset() {
    _runBlocks.clear();

    _processed = 0;
    _pending = 0;
    _loop = 0;
    _inLoop = false;

    _sequenceBlocks.clear();
    _sequenceBlocks.emplace_back(0, -1);

    _lastOccurenceBeforeLoop.clear();
    _rotationEqualityCache.clear();
}

RunBlockSequenceNode* RunSummaryBase::getChildNode(
    RunBlockSequenceNode* parent, RunUnitId targetId, size_t start
) {
    RunBlockSequenceNode* node = parent;

    if (!node->_childIndex) {
        // This node does not yet have any children. Add the first.
        node->_childIndex = _sequenceBlocks.size();
        _sequenceBlocks.emplace_back(targetId, start);
        return &_sequenceBlocks.back();
    }

    node = &_sequenceBlocks[node->_childIndex];
    if (node->getRunUnitId() == targetId) {
        // Found it!
        return node;
    }

    // Check the siblings
    while (node->_siblingIndex) {
        node = &_sequenceBlocks[node->_siblingIndex];
        if (node->getRunUnitId() == targetId) {
            // Found it!
            return node;
        }
    }

    node->_siblingIndex = _sequenceBlocks.size();
    _sequenceBlocks.emplace_back(targetId, start);
    return &_sequenceBlocks.back();
}

size_t RunSummaryBase::sequenceId(size_t start, size_t end) {
    RunBlockSequenceNode* sequenceNodeP = &_sequenceBlocks.at(0);

    for (size_t i = start; i < end; ++i) {
        sequenceNodeP = getChildNode(sequenceNodeP, getRunUnitIdAt(i), start);
    }

    return (int)(sequenceNodeP - &_sequenceBlocks[0]);
}

void RunSummaryBase::addRunBlock(size_t start, size_t sequenceId, size_t loopPeriod) {
    if (loopPeriod && _runBlocks.size()) {
        _lastOccurenceBeforeLoop[_runBlocks.back().getSequenceId()] = _runBlocks.size() - 1;
    }
    _runBlocks.emplace_back(start, sequenceId, loopPeriod);
}

void RunSummaryBase::createRunBlock(size_t start, size_t end, size_t loopPeriod) {
    addRunBlock(start, sequenceId(start, end), loopPeriod);
}

void RunSummaryBase::createRunBlocks(size_t start, size_t end) {
    if (start == end) return;

    if (!_identifyShortLoops || !_runBlocks.size()) {
        createRunBlock(start, end, 0);
        return;
    }

    auto prevBlock = getLastRunBlock();
    assert(prevBlock->isLoop());

    RunBlockSequenceNode* sequenceNodeP = &_sequenceBlocks.at(0);
    size_t prevSeqId = prevBlock->getSequenceId();

    size_t mid = start;
    while (mid != end) {
        // Find last time when this run block was followed by another loop
        auto result = _lastOccurenceBeforeLoop.find(prevSeqId);

        if (result != _lastOccurenceBeforeLoop.end()) {
            // Check if the run history matches the loop
            auto& loopBlock = _runBlocks[result->second + 1];
            assert(loopBlock.isLoop());

            auto seqNode = _sequenceBlocks[loopBlock.getSequenceId()];
            size_t loopStart = seqNode._startIndex;
            size_t loopPeriod = loopBlock.getLoopPeriod();

            size_t matchLen = 0;
            while (mid + matchLen < end &&
                   getRunUnitIdAt(mid + matchLen) ==
                   getRunUnitIdAt(loopStart + matchLen % loopPeriod)) {
                ++matchLen;
            }

            if (matchLen > 0) {
                // Create (short) loop run block for this sequence

                if (mid != start) {
                    // First add run block for sequence that precedes it
                    createRunBlock(start, mid, 0);
                }

                addRunBlock(mid, loopBlock.getSequenceId(), loopPeriod);

                createRunBlocks(mid + matchLen, end); // Recurse

                return;
            }
        }

        // Failed to start a loop from here. Add it to preceding sequence
        sequenceNodeP = getChildNode(sequenceNodeP, getRunUnitIdAt(mid), start);
        prevSeqId = static_cast<int>(sequenceNodeP - &_sequenceBlocks[0]);
        mid += 1;
    }

    createRunBlock(start, end, 0);
}

void RunSummaryBase::resetPending() {
    auto rb = getLastRunBlock();
    assert(rb->isLoop());
    _loop = rb->getStartIndex();
    _processed = _loop + 2 * rb->getLoopPeriod();
    _inLoop = true;
}

size_t RunSummaryBase::getRunBlockLength(size_t startIndex, size_t endIndex) const {
    bool isLast = endIndex == getNumRunBlocks();
    size_t runUnitStartIndex = _runBlocks[startIndex].getStartIndex();

    if (isLast) {
        return (_inLoop ? _processed : _pending) - runUnitStartIndex;
    } else {
        return _runBlocks[endIndex].getStartIndex() - runUnitStartIndex;
    }
}

std::optional<size_t> RunSummaryBase::findLoopOfLength(size_t sequenceId, size_t length) const {
    assert(sequenceId < _sequenceBlocks.size());

    auto it = _runBlocks.rbegin();
    if (it == _runBlocks.rend()) return {};

    // Handle last run block as special case
    if (it->getSequenceId() == sequenceId) {
        size_t len = (_inLoop ? _processed : _pending) - it->getStartIndex();
        if (len >= length) {
            return &*it - &_runBlocks[0];
        }
    }

    auto rbNext = it;
    while (++it != _runBlocks.rend()) {
        if (it->getSequenceId() == sequenceId) {
            size_t len = rbNext->getStartIndex() - it->getStartIndex();
            if (len >= length) {
                return &*it - &_runBlocks[0];
            }
        }
        rbNext = it;
    }

    return {};
}

size_t RunSummaryBase::getLoopIteration() const {
    assert(_inLoop);

    auto &loopBlock = _runBlocks.back();
    size_t startIndex = loopBlock.getStartIndex();
    size_t loopLength = getNumRunUnits() - startIndex;

    return loopLength / loopBlock.getLoopPeriod();
}

bool RunSummaryBase::isAtEndOfLoop() const {
    assert(_inLoop);

    auto &loopBlock = _runBlocks.back();
    size_t startIndex = loopBlock.getStartIndex();
    size_t loopLength = getNumRunUnits() - startIndex;

    return loopLength % loopBlock.getLoopPeriod() == 0;
}

// Implements Booth's algorithm
// See: https://en.wikipedia.org/wiki/Lexicographically_minimal_string_rotation#Booth's_Algorithm
size_t RunSummaryBase::calculateCanonicalLoopIndex(size_t startIndex, size_t len) const {
    // Initialize failure function
    std::vector<int> f(len, -1);

    size_t k = 0; // Least rotation of sequence found so far

    for (int j = 1; j < len; ++j) {
        size_t sj = getRunUnitIdAt(startIndex + j);
        int i = f[j - k - 1];
        while (i != -1 && sj != getRunUnitIdAt(startIndex + k + i + 1)) {
            if (sj < getRunUnitIdAt(startIndex + k + i + 1)) {
                k = j - i - 1;
            }
            i = f[i];
        }
        if (sj != getRunUnitIdAt(startIndex + k + i + 1)) {
            assert(i == -1);
            if (sj < getRunUnitIdAt(startIndex + k)) { // k + i + 1 == k
                k = j;
            }
            f[j - k] = -1;
        } else {
            f[j - k] = i + 1;
        }
    }

    return k;
}

bool RunSummaryBase::determineRotationEquivalence(size_t index1, size_t index2, size_t len,
                                                  size_t &offset) const {
    size_t relIndex1 = calculateCanonicalLoopIndex(index1, len);
    size_t relIndex2 = calculateCanonicalLoopIndex(index2, len);

    size_t ci1 = index1 + relIndex1;
    size_t ci2 = index2 + relIndex2;
    for (size_t i = 0; i < len; ++i) {
        if (getRunUnitIdAt(ci1 + i) != getRunUnitIdAt(ci2 + i)) {
            return false;
        }
    }

    offset = (relIndex1 + len - relIndex2) % len;

    return true;
}


bool RunSummaryBase::areLoopsRotationEqual(const RunBlock* block1, const RunBlock* block2,
                                           size_t &indexOffset) const {
    size_t index1 = block1->getSequenceId();
    size_t index2 = block2->getSequenceId();
    if (index1 == index2) {
        // Blocks are equal, even without rotating
        return true;
    }

    // Require that run blocks are loops. This avoids modular arithmetic when executing Booth's
    // algorithm.
    assert(block1->isLoop());
    assert(block2->isLoop());

    size_t len = block1->getLoopPeriod();
    if (len != block2->getLoopPeriod()) {
        return false;
    }

    auto &map = _rotationEqualityCache;
    auto key = std::make_pair(index1, index2);
    auto cachedResult = map.find(key);
    if (cachedResult != map.end()) {
        // Return previously calculated result
        indexOffset = cachedResult->second.second;
        return cachedResult->second.first;
    }

    bool areEqual = determineRotationEquivalence(block1->getStartIndex(),
                                                 block2->getStartIndex(),
                                                 len, indexOffset);
    // Cache result (and its equivalent inverse)
    map[key] = std::make_pair(areEqual, indexOffset);
    map[std::make_pair(index2, index1)] = std::make_pair(areEqual, len - indexOffset);

    return areEqual;
}

void RunSummaryBase::dumpRunBlockSequenceNode(size_t nodeIndex, size_t level) const {
    for (size_t i = 0; i < level; i++) {
        std::cout << "  ";
    }

    auto &node = _sequenceBlocks[nodeIndex];
    std::cout << nodeIndex << " (" << node.getRunUnitId() << ")" << std::endl;

    // Dump children
    if (node._childIndex) {
        dumpRunBlockSequenceNode(node._childIndex, level + 1);
    }

    // Dump siblings
    if (node._siblingIndex) {
        dumpRunBlockSequenceNode(node._siblingIndex, level);
    }
}

void RunSummaryBase::dumpSequenceTree() const {
    dumpRunBlockSequenceNode(0, 0);
}

void RunSummaryBase::dumpCondensed(bool hideLegend) const {
    for (size_t i = 0; i < getNumRunBlocks(); i++) {
        const RunBlock* runBlock = runBlockAt(i);
        size_t sequenceIndex = runBlock->getSequenceId();

        if (i > 0) {
            std::cout << " ";
        }

        std::cout << "#" << sequenceIndex;

        if (runBlock->isLoop()) {
            size_t len = getRunBlockLength(i);
            size_t period = runBlock->getLoopPeriod();

            std::cout << "*" << len / period << "." << (len % period);
        }
    }
    std::cout << std::endl;

    if (hideLegend) return;

    size_t numUnique = 0;
    size_t helperArray[64];

    std::cout << "with:" << std::endl;
    for (size_t i = 0; i < getNumRunBlocks(); i++) {
        const RunBlock* runBlock = runBlockAt(i);
        size_t sequenceIndex = runBlock->getSequenceId();

        size_t j = 0;
        while (j < numUnique && helperArray[j] != sequenceIndex) {
            j++;
        }

        if (j == numUnique) {
            // First encounter of this block. So dump it
            std::cout << sequenceIndex << " =";

            size_t len = getRunBlockLength(i);
            if (runBlock->isLoop()) {
                len = runBlock->getLoopPeriod();
            }

            size_t k = runBlock->getStartIndex();
            size_t end = k + len;
            while (k < end) {
                std::cout << " " << getRunUnitIdAt(k++);
            }
            std::cout << std::endl;

            helperArray[numUnique++] = sequenceIndex;
            if (numUnique == 64) {
                return;
            }
        }
    }
}

void RunSummaryBase::dump() const {
    size_t runUnitIndex = 0;
    size_t numRunUnits = getNumRunUnits();
    auto runBlock = _runBlocks.cbegin();
    std::optional<size_t> numPendingBlocks;
    bool isLoop = false;

    while (runUnitIndex < numRunUnits) {
        if (numPendingBlocks) {
            if (--(*numPendingBlocks) == 0) {
                std::cout << (isLoop ? "] " : ") ");
                numPendingBlocks.reset();
            }
        }

        if (!numPendingBlocks) {
            assert(runBlock != _runBlocks.cend());
            assert((*runBlock).getStartIndex() == runUnitIndex);

            isLoop = (*runBlock).isLoop();
            std::cout << "#" << (*runBlock).getSequenceId() << (isLoop ? "[" : "(");
            numPendingBlocks = getRunBlockLength(&*runBlock);

            runBlock++;
        } else {
            std::cout << " ";
        }
        std::cout << getRunUnitIdAt(runUnitIndex++);
    }
    std::cout << std::endl;
}

size_t RunSummary::getDpDeltaOfProgramBlockSequence(size_t start, size_t end) const {
    size_t dpDelta = 0;

    for (auto &programBlock : makeRange(_runHistory, start, end)) {
        if (!programBlock->isDelta()) {
            dpDelta += programBlock->getInstructionAmount();
        }
    }

    return dpDelta;
}

size_t RunSummary::getDpDelta(size_t firstRunBlock, size_t lastRunBlock) const {
    size_t dpDelta = 0;

    for (auto& runBlock : makeRange(_runBlocks, firstRunBlock, lastRunBlock)) {
        size_t runBlockLen = getRunBlockLength(&runBlock);
        size_t start = runBlock.getStartIndex();

        if (runBlock.isLoop()) {
            size_t loopPeriod = runBlock.getLoopPeriod();
            size_t dpDeltaPerIter = getDpDeltaOfProgramBlockSequence(start, start + loopPeriod);
            size_t numIter = runBlockLen / loopPeriod;
            size_t fullSpinEnd = loopPeriod * numIter;

            dpDelta += numIter * dpDeltaPerIter;
            dpDelta += getDpDeltaOfProgramBlockSequence(start + fullSpinEnd, start + runBlockLen);
        } else {
            dpDelta += getDpDeltaOfProgramBlockSequence(start, start + runBlockLen);
        }
    }

    return dpDelta;
}

void MetaRunSummary::attemptLoopCollapse() {
    if (!_metaLoopDetector) {
        _metaLoopDetector = std::make_unique<MetaRunSummary>(getRunBlocks(), getHelperBuffer());
        _metaLoopIterations = 0;
    }

    _metaLoopDetector->processNewRunUnits();

    if (!_metaLoopDetector->isInsideLoop()) {
        return;
    }

    if (_metaLoopIterations == _metaLoopDetector->getLoopIteration()) {
        return;
    }

    _metaLoopIterations = _metaLoopDetector->getLoopIteration();

    auto metaRunBlock = _metaLoopDetector->getLastRunBlock();
    assert(metaRunBlock->isLoop());

    // Period in old run-blocks
    size_t loopPeriod = metaRunBlock->getLoopPeriod();
    size_t loopStart = metaRunBlock->getStartIndex() + loopPeriod * (_metaLoopIterations - 2);

//    std::cout << "Before: ";
//    dumpCondensed();
//    _metaLoopDetector->dump();

    // The meta-loop detector ignores varying run length of loops. For collapse at this level
    // the run units need to exactly repeat. Check if this is the case.
    for (size_t i = 0; i < loopPeriod; ++i) {
        if (!runBlockAt(loopStart + i)->isLoop()) {
            continue;
        }

        size_t len1 = getRunBlockLength(loopStart + i);
        size_t len2 = getRunBlockLength(loopStart + loopPeriod + i);

        if (len1 != len2) {
            // Cannot re-group as run unit history does not repeat exactly
            return;
        }
    }

    // Remove previous blocks
    _runBlocks.erase(_runBlocks.begin() + loopStart, _runBlocks.end());

    // Replace by bigger loop-block
    size_t unitStart = runBlockAt(loopStart)->getStartIndex();
    size_t unitLoopPeriod = getRunBlockLength(loopStart, loopStart + loopPeriod);

    createRunBlock(unitStart, unitStart + 2 * unitLoopPeriod, unitLoopPeriod);

    resetPending();

//    std::cout << "After: ";
//    dumpCondensed();

    _rewriteCount += 1;
    _metaLoopDetector.reset();
}

void MetaRunSummary::exitedLoop() {
    attemptLoopCollapse();
}

void MetaRunSummary::reset() {
    RunSummaryBase::reset();

    _metaLoopDetector.reset();
    _rewriteCount = 0;
}
