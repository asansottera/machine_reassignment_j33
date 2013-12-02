#include "heuristic_factory.h"
#include "local_search.h"
#include "fast_local_search.h"
#include "simulated_annealing.h"
#include "vns.h"
#include "best_improvement_local_search.h"
#include "first_improvement_local_search.h"
#include "multi_start_local_search.h"
#include "els.h"
#include "path_relinking.h"
#include "tabu_search.h"
#include "vns2.h"
#include "random_exchange_ls.h"
#include "linear_solver.h"
#include "random_move_ls.h"
#include "vns3.h"

using namespace R12;

Heuristic * R12::makeHeuristic(const std::string & name) {
	if (name.compare("local_search") == 0) {
		return new LocalSearch();
	} else if (name.compare("fast_local_search") == 0) {
		return new FastLocalSearch();
	} else if (name.compare("vns") == 0) {
		return new VNS();
	} else if (name.compare("simulated_annealing") == 0) {
		return new SimulatedAnnealing();
	} else if (name.compare("best_improvement_local_search") == 0) {
		return new BestImprovementLocalSearch();
	} else if (name.compare("first_improvement_local_search") == 0) {
		return new FirstImprovementLocalSearch();
	} else if (name.compare("multi_start_local_search") == 0) {
		return new MultiStartLocalSearch();
	} else if (name.compare("els") == 0) {
		return new ELS();
	} else if (name.compare("path_relinking") == 0) {
		return new PathRelinking();
	} else if (name.compare("tabu_search") == 0) {
		return new TabuSearch();
	} else if (name.compare("vns2") == 0) {
		return new VNS2();
	} else if (name.compare("random_exchange_ls") == 0) {
		return new RandomExchangeLS();
	} else if (name.compare("linear_solver") == 0) {
		return new LinearSolver();
	} else if (name.compare("random_move_ls") == 0) {
		return new RandomMoveLS();
	} else if (name.compare("vns3") == 0) {
		return new VNS3();
	} else {
		throw std::runtime_error("Unknown heuristic");
	}
}

