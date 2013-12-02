#include "multi_start_local_search.h"
#include "first_improvement_local_search.h"

using namespace R12;

void MultiStartLocalSearch::run() {
	std::unique_ptr<Heuristic> lsPtr(new FirstImprovementLocalSearch());
	SolutionInfo startInfo(instance(), initial());
	m_bestObjective = startInfo.objective();
	m_bestSolution = startInfo.solution();
	uint32_t lsSeed = seed();
	while (!interrupted()) {
		SolutionInfo newStartInfo = startInfo;
		lsPtr->init(instance(), initial(), lsSeed++, flag(), pool(), "");
		lsPtr->runFromSolution(newStartInfo);
		if (interrupted()) {
			break;
		}
		if (lsPtr->bestObjective() < m_bestObjective) {
			m_bestObjective = lsPtr->bestObjective();
			m_bestSolution = lsPtr->bestSolution();
		}
	}
	pool().push(m_bestObjective, m_bestSolution);
	signalCompletion();
}

