#include "best_improvement_local_search.h"
#include "common.h"
#include "solution_info.h"
#include "move_verifier.h"
#include <stdexcept>
#include <queue>
#include <algorithm>
#include <cstdlib>
#include <sstream>

using namespace R12;

// #define CHECK_BILS
// #define TRACE_BILS

#ifdef CHECK_BILS
#include "verifier.h"
#endif

#ifdef TRACE_BILS
#include <iostream>
#endif

void BestImprovementLocalSearch::run() {
	SolutionInfo info(instance(), initial());
	runFromSolution(info);
	pool().push(m_bestObjective, m_bestSolution);
	signalCompletion();
}

void BestImprovementLocalSearch::runFromSolution(SolutionInfo & info) {
	MoveVerifier v(info);
	m_bestObjective = info.objective();
	#ifdef CHECK_BILS
	Verifier verifier;
	verifier.setExitOnFirstViolation(true);
	Verifier::Result result = verifier.verify(instance(), initial(), info.solution());
    	if (m_bestObjective != result.objective()) {
		throw std::runtime_error("Best improvement local search - Wrong objective at starting solution");
	}
	
	#endif
	// start local search
	bool continueLocalSearch = true;
	uint32_t iteration = 0;
	uint64_t moveCount = 0;
	while (continueLocalSearch && !interrupted()) {
		++iteration;
		#ifdef TRACE_BILS
		if (iteration % 100 == 0) {
			std::cout << "Best improvement local search - Best solution at @ iteration " << iteration << ": " << m_bestObjective << std::endl;
		}
		#endif
		continueLocalSearch = false;
		Move topMove(0, 0, 0);
		uint64_t topObj = m_bestObjective;
		for (ProcessID p = 0; p < instance().processes().size(); ++p) {
			MachineID src = info.solution()[p]; 
			for (MachineID m = 0; m < instance().machines().size(); ++m) {
				if (src != m) {
					++moveCount;
					Move move(p, src, m);
					bool feasible = v.feasible(move);
					#ifdef CHECK_BILS
					std::vector<MachineID> newSolution = info.solution();
					newSolution[p] = m;
					Verifier::Result result = verifier.verify(instance(), initial(), newSolution);
					if (feasible != result.feasible()) {
						std::stringstream msg;
						msg << "Best improvement local search - Wrong feasibility at iteration" << iteration << ", move " << moveCount;
						std::cout << msg.str() << std::endl;
						throw std::runtime_error(msg.str());
					}
					#endif
					if (feasible) {
						uint64_t objective = v.objective(move);
						#ifdef CHECK_BILS
						if (objective != result.objective()) {
							std::stringstream msg;
							msg << "Best improvement local search - Wrong objective at iteration " << iteration << ", move " << moveCount;
							std::cout << msg.str() << std::endl;
							throw std::runtime_error(msg.str());
						}
						#endif
						if (objective < topObj) {
							continueLocalSearch = true;
							topMove = move;
							topObj = objective;
						}
					}
				}
			} // end machine loop
			if (interrupted()) {
				break;
			}
		} // end process loop
		// make the best move for this process
		if (continueLocalSearch) {
			m_bestObjective = v.objective(topMove);
			v.commit(topMove);
			#ifdef CHECK_BILS
			if (!info.check()) {
				std::stringstream msg;
				msg << "Best improvement local search - Inconsistent solution at iteration " << iteration;
				std::cout << msg.str() << std::endl;
				throw std::runtime_error(msg.str());
			}
			#endif
		}
	} // end local search loop
	m_bestSolution = info.solution();
	#ifdef CHECK_BILS
	if (!info.check()) {
		throw std::runtime_error("Best improvement local search - Solution information is incosistent with assignment");
	}
	#endif
	#ifdef TRACE_BILS
	std::cout << "Best improvement local search - " << iteration << " iterations and " << moveCount << " moves evalutated" << std::endl;
	#endif
}

