#include "first_improvement_local_search.h"
#include "common.h"
#include "solution_info.h"
#include "verifier.h"
#include "move_verifier.h"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>

using namespace R12;

// #define CHECK_FILS
// #define TRACE_FILS

void FirstImprovementLocalSearch::run() {
	SolutionInfo info(instance(), initial());
	runFromSolution(info);
	pool().push(m_bestObjective, m_bestSolution);
	signalCompletion();
}

void FirstImprovementLocalSearch::runFromSolution(SolutionInfo & info) {
	MoveVerifier v(info);
	m_bestObjective = info.objective();
	#ifdef CHECK_FILS
	Verifier verifier;
	verifier.setExitOnFirstViolation(true);
	Verifier::Result result = verifier.verify(instance(), initial(), info.solution());
    	if (m_bestObjective != result.objective()) {
		std::stringstream msg;
		msg << "First improvement local search - Wrong objective at starting solution";
		std::cout << msg << std::endl;
		throw std::runtime_error(msg.str());
	}
	#endif
	// randomly sort processes and machines
	std::size_t pCount = instance().processes().size();
	std::size_t mCount = instance().machines().size();
	std::vector<ProcessID> processes(pCount);
	std::vector<MachineID> machines(mCount);
	for (ProcessID p = 0; p < pCount; ++p) {
		processes[p] = p;
	}
	for (MachineID m = 0; m < mCount; ++m) {
		machines[m] = m;
	}
	srand(seed());
	// start local search
	bool continueLocalSearch = true;
	uint32_t iteration = 0;
	uint64_t moveCount = 0;
	while (continueLocalSearch && !interrupted()) {
		++iteration;
		// randomly sort processes and machines
		std::random_shuffle(processes.begin(), processes.end());
		std::random_shuffle(machines.begin(), machines.end());
		#ifdef TRACE_FILS
		if (iteration % 100 == 0) {
			std::cout << "First improvement local search - Best solution at @ iteration " << iteration << ": " << m_bestObjective << std::endl;
		}
		#endif
		continueLocalSearch = false;
		bool nextIteration = false;
		for (auto pItr = processes.begin(); pItr != processes.end(); ++pItr) {
			ProcessID p = *pItr;
			for (auto mItr = machines.begin(); mItr != machines.end(); ++mItr) {
				MachineID m = *mItr;
				MachineID old_m = info.solution()[p];
				if (old_m != m) {
					++moveCount;
					Move move(p, old_m, m);
					bool feasible = v.feasible(move);
					#ifdef CHECK_FILS
					std::vector<MachineID> newSolution = info.solution();
					newSolution[p] = m;
					Verifier::Result result = verifier.verify(instance(), initial(), newSolution);
					if (feasible != result.feasible()) {
						std::stringstream msg;
						msg << "First improvement local search - Wrong feasibility at iteration" << iteration << ", move " << moveCount;
						std::cout << msg << std::endl;
						throw std::runtime_error(msg.str());
					}
					#endif
					if (feasible) {
						uint64_t objective = v.objective(move);
						#ifdef CHECK_FILS
						if (objective != result.objective()) {
							std::stringstream msg;
							msg << "First improvement local search - Wrong objective at iteration " << iteration << ", move " << moveCount;
							std::cout << msg << std::endl;
							throw std::runtime_error(msg.str());
						}
						#endif
						if (objective < m_bestObjective) {
							m_bestObjective = objective;
							continueLocalSearch = true;
							nextIteration = true;
							v.commit(move);
							#ifdef CHECK_FILS
							if (!info.check()) {
								std::stringstream msg;
								msg << "First improvement local search - Inconsistent solution at iteration " << iteration << ", move " << moveCount;
								std::cout << msg << std::endl;
								throw std::runtime_error(msg.str());
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
	#ifdef TRACE_FILS
	std::cout << "First improvement local search - " << iteration << " iterations and " << moveCount << " moves evalutated" << std::endl;
	#endif
}

