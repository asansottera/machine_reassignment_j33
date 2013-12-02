#ifndef R12_BALANCE_COST_OPTIMIZER_H
#define R12_BALANCE_COST_OPTIMIZER_H

#include "common.h"
#include "solution_info.h"

namespace R12 {

class BalanceCostOptimizer {
	void optimizeBalance(SolutionInfo & x, const BalanceCostID b) const;
public:
	void optimize(SolutionInfo & x) const;
};

}

#endif
