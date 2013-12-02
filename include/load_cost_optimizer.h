#ifndef R12_LOAD_COST_OPTIMIZER_H
#define R12_LOAD_COST_OPTIMIZER_H

#include "common.h"
#include "solution_info.h"

namespace R12 {

class LoadCostOptimizer {
	void optimizeResource(SolutionInfo & x, const ResourceID r) const;
	uint64_t evaluateLoadCost(const Machine & machine,
							  const ResourceID r,
							  const int64_t usage) const;
public:
	void optimize(SolutionInfo & x) const;
};

}

#endif
