#ifndef R12_FIRST_IMPROVEMENT_LOCAL_SEARCH_H
#define R12_FIRST_IMPROVEMENT_LOCAL_SEARCH_H

#include "heuristic.h"
#include "problem.h"
#include "verifier.h"
#include "solution_info.h"
#include <memory>

namespace R12 {

/*! Performs a first improvement local search. */
class FirstImprovementLocalSearch : public Heuristic {
private:
	std::vector<MachineID> m_bestSolution;
	uint64_t m_bestObjective;
public:
	FirstImprovementLocalSearch() {
		m_bestObjective = 0;
	}
	void runFromSolution(SolutionInfo & info);
	virtual void run();
	virtual const std::vector<MachineID> & bestSolution() const { return m_bestSolution; }
	virtual uint64_t bestObjective() const { return m_bestObjective; }
};

}

#endif
