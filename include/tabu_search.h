#ifndef R12_TABU_SEARCH_H
#define R12_TABU_SEARCH_H

#include <boost/random/taus88.hpp>
#include <deque>
#include <unordered_set>
#include <vector>

#include "common.h"
#include "heuristic.h"

namespace R12
{

class TabuSearch : public Heuristic {
public:
	TabuSearch() : best_objective(0) {}
	~TabuSearch() {}
	void run();
	const std::vector<MachineID>& bestSolution() const { return best_solution; }
	uint64_t bestObjective() const { return best_objective; }
private:
	std::vector<MachineID> best_solution;
	uint64_t best_objective;

	boost::random::taus88 randomizer;
	std::unordered_set<ProcessID> tabu_processes;
	std::deque<ProcessID> tabu_processes_order;
	std::unordered_set<MachineID> tabu_machines;
	std::deque<MachineID> tabu_machines_order;
private:
	TabuSearch(const TabuSearch&);
	TabuSearch& operator=(const TabuSearch&);

	// XXX: don't call this methods before having initialized the
	// randomizer with the seed
	inline ProcessID pickProcess();
	inline MachineID pickMachine();
};

}

#endif // R12_TABU_SEARCH_H

