//
//  SearchOrchestrator.h
//  BusyBeaverFinder
//
//  Created by Erwin on 11/02/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include <string>

#include "Types.h"

class ExhaustiveSearcher;

void orchestratedSearch(ExhaustiveSearcher& searcher);

void searchLateEscapes(ExhaustiveSearcher& searcher, std::string lateEscapesFile);
