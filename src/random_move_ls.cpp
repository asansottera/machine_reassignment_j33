#include "random_move_ls.h"
#include "move_verifier.h"
#include <boost/random.hpp>

//#define CHECK_RANDOM_MOVE_LS
#define TRACE_RANDOM_MOVE_LS 1

#ifdef CHECK_RANDOM_MOVE_LS
#include "verifier.h"
#include <stdexcept>
#endif

using namespace std;
using namespace R12;

void RandomMoveLS::run() {
	SolutionInfo info(instance(), initial());
	#ifdef CHECK_RANDOM_MOVE_LS
	Verifier v;
	#endif
	#ifdef TRACE_RANDOM_MOVE_LS
	uint64_t evalCount = 0;
	uint64_t feasibleCount = 0;
	uint64_t commitCount = 0;
	#endif
	MoveVerifier mv(info);
	boost::mt19937 rng(seed());
	boost::uniform_int<ProcessID> pdist(0, instance().processes().size() - 1);
	boost::uniform_int<ProcessID> mdist(0, instance().machines().size() - 1);
	uint64_t it = 0;
	while (!interrupted()) {
		const ProcessID p = pdist(rng);
		const MachineID src = info.solution()[p];
		const MachineID dst = mdist(rng);
		if (src != dst) {
			++it;
			const Move move(p, src, dst);
			#if TRACE_RANDOM_MOVE_LS >= 1
			++evalCount;
			#endif
			bool feasible = mv.feasible(move);
			#ifdef CHECK_RANDOM_MOVE_LS
			std::vector<MachineID> newSolution = info.solution();
			newSolution[p] = dst;
			Verifier::Result result = v.verify(instance(), initial(), newSolution);
			if (feasible != result.feasible()) {
				throw std::logic_error("MoveVerifier disagrees with Verifier (feasibility)");
			}
			CHECK(feasible == result.feasible());
			#endif
			if (feasible) {
				#if TRACE_RANDOM_MOVE_LS >= 1
				++feasibleCount;
				#endif
				uint64_t obj = mv.objective(move);
				#ifdef CHECK_RANDOM_MOVE_LS
				if (obj != result.objective()) {
					throw std::logic_error("MoveVerifier disagrees with Verifier (objective value)");
				}
				#endif
				if (obj < info.objective()) {
					#if TRACE_RANDOM_MOVE_LS >= 1
					++commitCount;
					#endif
					mv.commit(move);
					#ifdef CHECK_RANDOM_MOVE_LS
					if (!info.check()) {
						throw std::logic_error("MoveVerifier did not update SolutionInfo correctly");
					}
					#endif
					#if TRACE_RANDOM_MOVE_LS >= 2
					std::cout << "Random Move Local Search - ";
					std::cout << "Improvement found @ iteration " << it << ": ";
					std::cout << obj << std::endl;
					std::cout << "Random Move Local Search - Statistics @ iteration " << it << ": ";
					std::cout << "feasible ratio = ";
					std::cout << static_cast<double>(feasibleCount) / static_cast<double>(evalCount) << ", ";
					std::cout << "commit ratio = ";
					std::cout << static_cast<double>(commitCount) / static_cast<double>(evalCount) << std::endl;
					#endif
					m_bestObjective = obj;
					m_bestSolution = info.solution();
				}
			}
		}
	}
	#if TRACE_RANDOM_MOVE_LS >= 1
	std::cout << "Random Move Local Search - Statistics: ";
	std::cout << "feasible ratio = ";
	std::cout << static_cast<double>(feasibleCount) / static_cast<double>(evalCount) << ", ";
	std::cout << "commit ratio = ";
	std::cout << static_cast<double>(commitCount) / static_cast<double>(evalCount) << ", ";
	std::cout << "evaluated moves = " << evalCount << std::endl;
	#endif
	pool().push(info.objective(), info.solution());
	signalCompletion();
}
