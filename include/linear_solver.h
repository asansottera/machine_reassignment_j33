#ifndef R12_LINEAR_SOLVER_H
#define R12_LINEAR_SOLVER_H

#include <vector>
#include <glpk.h>
#include "common.h"
#include "heuristic.h"
#include "linear_solver_sub_problem.h"



namespace R12
{

class LinearSolver : public Heuristic {
	
public:
	LinearSolver() {};
	~LinearSolver() {};
	void run();
	const std::vector<MachineID>& bestSolution() const { return best_solution; }
	uint64_t bestObjective() const { return best_objective; }
	
private:

	ServiceID pickService();
		
	boost::random::taus88 randomizer;

	
	std::vector<MachineID> best_solution;
	uint32_t best_objective;

	uint32_t n_processes;
	uint32_t n_machines;
	uint32_t n_resources;
	uint32_t n_services;
	uint32_t n_neighborhoods;
	uint32_t n_locations;
	uint32_t n_balanceCosts;

	
};

}

#endif //R12_LINEAR_SOLVER_H
