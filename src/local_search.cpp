#include "local_search.h"
#include "common.h"
#include <iostream>

using namespace R12;

void LocalSearch::run() {
	m_bestSolution = initial();
	m_bestObjective = m_verifier.verify(instance(), initial(), m_bestSolution).objective();
	bool continueLocalSearch = true;
	uint32_t iteration = 0;
	uint64_t move = 0;
	while (continueLocalSearch && !interrupted()) {
		++iteration;
		continueLocalSearch = false;
		bool nextIteration = false;
		for (ProcessID p = 0; p < instance().processes().size(); ++p) {
			for (MachineID m = 0; m < instance().machines().size(); ++m) {
				MachineID old_m = m_bestSolution[p];
				if (old_m != m) {
					++move;
					m_bestSolution[p] = m;
					Verifier::Result result = m_verifier.verify(instance(), initial(), m_bestSolution);
					if (result.feasible() && result.objective() < m_bestObjective) {
						m_bestObjective = result.objective();
						std::cout << "Local search - Improvement found @ iteration " << iteration << ": " << m_bestObjective << std::endl;
						continueLocalSearch = true;
						nextIteration = true;
					} else {
						m_bestSolution[p] = old_m;
					}
				}
				if (nextIteration || interrupted()) {
					break;
				}
			} // end machine loop
			if (nextIteration || interrupted()) {
				break;
			}
		} // end process loop
	} // end local search loop
	std::cout << "Local search - " << iteration << " iterations and " << move << " moves evalutated" << std::endl;
	signalCompletion();
}

