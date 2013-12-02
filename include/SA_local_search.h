#ifndef R12_SA_LOCAL_SEARCH_H
#define R12_SA_LOCAL_SEARCH_H

#include "heuristic_routine.h"
#include "solution_info.h"

namespace R12 {

class SALocalSearch :public HeuristicRoutine
{
public:
	virtual void SA_search(SolutionInfo & info, uint64_t bestObjective, long double MaxT, long double Temperature) = 0;
};

}

#endif