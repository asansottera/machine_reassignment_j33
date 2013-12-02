#include "fast_local_search.h"
#include "common.h"
#include "solution_info.h"
#include "verifier.h"
#include "move_verifier.h"
#include <iostream>
#include <stdexcept>

using namespace R12;

// #define CHECK_FLS

void FastLocalSearch::run() {
	SolutionInfo info(instance(), initial());
	runFromSolution(info);
	pool().push(m_bestObjective, m_bestSolution);
	signalCompletion();
}

void FastLocalSearch::runFromSolution(SolutionInfo & info) {
	MoveVerifier v(info);
	m_bestObjective = info.objective();
	#ifdef CHECK_FLS
	Verifier verifier;
	verifier.setExitOnFirstViolation(true);
	Verifier::Result result = verifier.verify(instance(), initial(), info.solution());
    	if (m_bestObjective != result.objective()) {
        	std::cout << "Fast local search - Wrong objective at starting solution" << std::endl;
	        throw std::runtime_error("Fast local search - Wrong objective");
	    }
	
	#endif
	// start local search
	bool continueLocalSearch = true;
	uint32_t iteration = 0;
	uint64_t moveCount = 0;
	while (continueLocalSearch && !interrupted()) {
		++iteration;
		if (iteration % 100 == 0) {
			std::cout << "Fast local search - Best solution at @ iteration " << iteration << ": " << m_bestObjective << std::endl;
		}
		continueLocalSearch = false;
		bool nextIteration = false;
		for (ProcessID p = 0; p < instance().processes().size(); ++p) {
			for (MachineID m = 0; m < instance().machines().size(); ++m) {
				MachineID old_m = info.solution()[p];
				if (old_m != m) {
					++moveCount;
					Move move(p, old_m, m);
					bool feasible = v.feasible(move);
					#ifdef CHECK_FLS
					std::vector<MachineID> newSolution = info.solution();
					newSolution[p] = m;
					Verifier::Result result = verifier.verify(instance(), initial(), newSolution);
					if (feasible != result.feasible()) {
						std::cout << "Fast local search - Wrong feasibility at iteration" << iteration << ", move " << moveCount << std::endl;
						throw std::runtime_error("Fast local search - Wrong feasibility");
					}
					#endif
					if (feasible) {
						uint64_t objective = v.objective(move);
						#ifdef CHECK_FLS
						if (objective != result.objective()) {
							std::cout << "Fast local search - Wrong objective at iteration " << iteration << ", move " << moveCount << std::endl;
							throw std::runtime_error("Fast local search - Wrong objective");
						}
						#endif
						if (objective < m_bestObjective) {
							m_bestObjective = objective;
							continueLocalSearch = true;
							nextIteration = true;
							v.commit(move);
							#ifdef CHECK_FLS
							if (!info.check()) {
								std::cout << "Fast local search - Inconsistent solution at iteration " << iteration << ", move " << moveCount << std::endl;
								throw std::runtime_error("Fast local search - Inconsistent solution");
							}
							#endif
						}
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
	m_bestSolution = info.solution();
	pool().push(m_bestObjective, m_bestSolution);
	#ifdef CHECK_FLS
	if (!info.check()) {
		throw std::runtime_error("Solution information is incosistent with assignment");
	}
	#endif
	std::cout << "Fast local search - " << iteration << " iterations and " << moveCount << " moves evalutated" << std::endl;
}

