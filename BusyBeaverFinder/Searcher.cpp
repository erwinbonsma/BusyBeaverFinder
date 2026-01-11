//
//  Searcher.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 11/01/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//

#include "Searcher.h"

Searcher::Searcher(ProgramSize size) : _program(size) {}

void Searcher::attachProgressTracker(std::unique_ptr<ProgressTracker> tracker) {
    _tracker = std::move(tracker);

    _tracker->setSearcher(this);
}

std::unique_ptr<ProgressTracker> Searcher::detachProgressTracker() {
    _tracker->setSearcher(nullptr);

    return std::move(_tracker);
}
