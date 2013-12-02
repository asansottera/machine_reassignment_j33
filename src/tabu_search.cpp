#include <tabu_search.h>

#include <boost/random/uniform_int_distribution.hpp>
#include <iostream>

#include <solution_info.h>
#include <move_verifier.h>

#define TABU_PROCESSES_MAX_SIZE (50)
#define TABU_MACHINES_MAX_SIZE (50)

#define TABU_SEARCH_WIPE (500)

#define TABU_SEARCH_SYNC (1000)

using namespace R12;


inline ProcessID TabuSearch::pickProcess()
{
	boost::random::uniform_int_distribution<ProcessID> distribution(0, instance().processes().size() - 1);
	return distribution(randomizer);
}

inline MachineID TabuSearch::pickMachine()
{
	boost::random::uniform_int_distribution<MachineID> distribution(0, instance().machines().size() - 1);
	return distribution(randomizer);
}

void TabuSearch::run()
{
	randomizer.seed(seed());

	SolutionInfo solution(instance(), initial());
	best_objective = solution.objective();
	best_solution = solution.solution();

	uint64_t i;
	for (i = 0; !interrupted(); i++) {
		if (i % TABU_SEARCH_WIPE == 0) {
			tabu_processes.clear();
			tabu_processes_order.clear();
			tabu_machines.clear();
			tabu_machines_order.clear();
		} else if (i % TABU_SEARCH_SYNC == 0) {
			SolutionPool::Entry entry;
			if (pool().best(entry)) {
				solution = SolutionInfo(instance(), initial(), *(entry.ptr()));
				best_objective = solution.objective();
				best_solution = solution.solution();
				// XXX: just reset the tabu lists
				tabu_processes.clear();
				tabu_processes_order.clear();
				tabu_machines.clear();
				tabu_machines_order.clear();
			}
		}
		MoveVerifier verifier(solution);
		ProcessID p = pickProcess();
		if (tabu_processes.find(p) != tabu_processes.end())
			continue;
		MachineID src = best_solution[p];
		MachineID dst;
		// FIXME: maybe this loop could lead to timing troubles...
		do {
			dst = pickMachine();
		} while (dst == src);
		if (tabu_machines.find(dst) != tabu_machines.end())
			continue;
		Move m(p, src, dst);
		if (verifier.feasible(m)) {
			uint64_t objective = verifier.objective(m);
			if (objective < best_objective) {
				verifier.commit(m);
				best_objective = objective;
				best_solution = solution.solution();
				if (tabu_processes.size() > TABU_PROCESSES_MAX_SIZE) {
					ProcessID lra_p = tabu_processes_order.front();
					tabu_processes_order.pop_front();
					tabu_processes.erase(lra_p);
				}
				tabu_processes.insert(p);
				tabu_processes_order.push_back(p);
				if (tabu_machines.size() > TABU_MACHINES_MAX_SIZE) {
					MachineID lra_m = tabu_machines_order.front();
					tabu_machines_order.pop_front();
					tabu_machines.erase(lra_m);
				}
				tabu_machines.insert(dst);
				tabu_machines_order.push_back(dst);

				pool().push(best_objective, best_solution);
			}
		}
	}

	// std::cout << "iteration: " << i << " objective: " << best_objective << std::endl;
	// std::cout << "iteration: " << i << " solution: ";
	// for (auto i = best_solution.begin(); i != best_solution.end(); i++)
	//	std::cout << *i << " ";
	// std::cout << std::endl;

	signalCompletion();
}

