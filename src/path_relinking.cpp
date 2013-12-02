#include "path_relinking.h"
#include "move_verifier.h"
#include "best_improvement_local_search.h"

#include <limits>

//#define TRACE_PATH_RELINKING

#ifdef TRACE_PATH_RELINKING
#include <iostream>
#endif

using namespace R12;

void PathRelinking::run() {
	// initialize best
	SolutionInfo initialInfo(instance(), initial());
	m_bestObjective = initialInfo.objective();
	m_bestSolution = initial();
	// subscribe to solution pool
	auto subPtr = pool().subscribe();
	// pair of solution to extract from the solution pool
	SolutionPool::Entry s1;
	SolutionPool::Entry s2;
	// run path relinking until termination signal
	uint64_t runs = 0;
	while (!interrupted()) {
		// wait for notification
		if (subPtr->wait()) {
			// successful!
			// retrieve another solution among the good ones
			pool().randomHighQuality(s1);
			// retrieve solution
			s2 = subPtr->dequeue();
			// check that the solutions are different enough
			if (delta(*s1.ptr(), *s2.ptr()) >= 2) {
				++runs;
				// relink solutions
				if (s1.obj() < s2.obj()) {
					relink(s1, s2);
				} else {
					relink(s2, s1);
				}
			}
		}
	}
	signalCompletion();
}

bool PathRelinking::step(SolutionInfo & info1, const SolutionInfo & info2, std::list<ProcessID> & differences) {
	MoveVerifier mv(info1);
	// look for best move
	std::list<ProcessID>::iterator bestItr = differences.end();
	uint64_t bestObj = std::numeric_limits<uint64_t>::max();
	for (auto itr = differences.begin(); itr != differences.end(); ++itr) {
		ProcessID p = *itr;
		Move move(p, info1.solution()[p], info2.solution()[p]);
		bool feasible = mv.feasible(move);
		if (feasible) {
			uint64_t obj = mv.objective(move);
			if (obj < bestObj) {
				bestItr = itr;
				bestObj = obj;
			}
		}
	}
	// apply best move if found
	if (bestItr != differences.end()) {
		ProcessID p = *bestItr;
		Move move(p, info1.solution()[p], info2.solution()[p]);
		mv.objective(move);
		mv.commit(move);
		differences.erase(bestItr);
		return true;
	} else {
		// failed to find feasible move
		return false;
	}
}

void PathRelinking::relink(const SolutionPool::Entry s1, const SolutionPool::Entry s2) {
	const std::vector<MachineID> & v1 = *s1.ptr();
	const std::vector<MachineID> & v2 = *s2.ptr();
	// enumerate processes placed on different machines
	std::list<ProcessID> differences;
	for (ProcessID p = 0; p < instance().processes().size(); ++p) {
		if (v1[p] != v2[p]) {
			differences.push_back(p);
		}
	}
	// relink
	SolutionInfo info1(instance(), initial(), v1);
	SolutionInfo info2(instance(), initial(), v2);
	uint64_t initialBestObj = info1.objective();
	uint64_t bestObj = info1.objective();
	std::vector<MachineID> best = v1;
	uint32_t it = 0;
	while (differences.size() > 1 && !interrupted()) {
		bool success = false;
		if (it % 2 == 0) {
			success = step(info1, info2, differences);
			uint64_t obj = info1.objective();
			if (obj < bestObj) {
				best = info1.solution();
				bestObj = obj;
			}
		} else {
			success = step(info2, info1, differences);
			uint64_t obj = info2.objective();
			if (obj < bestObj) {
				best = info2.solution();
				bestObj = obj;
			}
		}
		if (!success) {
			break;
		}
	}
	// check if we had an improvement
	if (bestObj < initialBestObj) {
		#ifdef TRACE_PATH_RELINKING
		std::cout << "Path relinking - Solution improved: " << bestObj << " (from " << initialBestObj << ")" << std::endl;
		#endif
		// do local search on the best solution
		BestImprovementLocalSearch ls;
		ls.init(instance(), initial(), seed(), flag(), pool(), "");
		SolutionInfo bestInfo(instance(), initial(), best);
		ls.runFromSolution(bestInfo);
		#ifdef TRACE_PATH_RELINKING
		std::cout << "Path relinking - Local search: " << bestInfo.objective() << std::endl;
		#endif
		// compare with best in the pool
		SolutionPool::Entry poolBest;
		pool().best(poolBest);
		if (bestInfo.objective() < poolBest.obj() * 1.1) {
			pool().push(bestInfo.objective(), bestInfo.solution());
		}
		// compare with best of this heuristic
		if (bestInfo.objective() < m_bestObjective) {
			m_bestObjective = bestInfo.objective();
			m_bestSolution = bestInfo.solution();
		}
	}
}
