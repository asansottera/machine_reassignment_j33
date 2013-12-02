#include "random_exchange_ls.h"
#include "exchange_verifier.h"
#include <boost/random.hpp>

//#define CHECK_RANDOM_EXCHANGE_LS
#define TRACE_RANDOM_EXCHANGE_LS 1

#ifdef CHECK_RANDOM_EXCHANGE_LS
#include "verifier.h"
#include <stdexcept>
#endif

using namespace std;
using namespace R12;

void RandomExchangeLS::run() {
	SolutionInfo info(instance(), initial());
	#ifdef CHECK_RANDOM_EXCHANGE_LS
	Verifier v;
	#endif
	#if TRACE_RANDOM_EXCHANGE_LS >= 1
	uint64_t evalCount = 0;
	uint64_t feasibleCount = 0;
	uint64_t commitCount = 0;
	#endif
	ExchangeVerifier ev(info);
	boost::mt19937 rng(seed());
	boost::uniform_int<ProcessID> pdist(0, instance().processes().size() - 1);
	uint64_t it = 0;
	while (!interrupted()) {
		const ProcessID p1 = pdist(rng);
		const ProcessID p2 = pdist(rng);
		if (p1 != p2) {
			++it;
			const MachineID m1 = info.solution()[p1];
			const MachineID m2 = info.solution()[p2];
			const Exchange exchange(m1, p1, m2, p2);
			#if TRACE_RANDOM_EXCHANGE_LS >= 1
			++evalCount;
			#endif
			bool feasible = ev.feasible(exchange);
			#ifdef CHECK_RANDOM_EXCHANGE_LS
			std::vector<MachineID> newSolution = info.solution();
			newSolution[p1] = m2;
			newSolution[p2] = m1;
			Verifier::Result result = v.verify(instance(), initial(), newSolution);
			if (feasible != result.feasible()) {
				throw std::logic_error("ExchangeVerifier disagrees with Verifier (feasibility)");
			}
			CHECK(feasible == result.feasible());
			#endif
			if (feasible) {
				#if TRACE_RANDOM_EXCHANGE_LS >= 1
				++feasibleCount;
				#endif
				uint64_t obj = ev.objective(exchange);
				#ifdef CHECK_RANDOM_EXCHANGE_LS
				if (obj != result.objective()) {
					throw std::logic_error("ExchangeVerifier disagrees with Verifier (objective value)");
				}
				#endif
				if (obj < info.objective()) {
					#if TRACE_RANDOM_EXCHANGE_LS >= 1
					++commitCount;
					#endif
					ev.commit(exchange);
					#ifdef CHECK_RANDOM_EXCHANGE_LS
					if (!info.check()) {
						throw std::logic_error("ExchangeVerifier did not update SolutionInfo correctly");
					}
					#endif
					#if TRACE_RANDOM_EXCHANGE_LS >= 2
					std::cout << "Random Exchange Local Search - ";
					std::cout << "Improvement found @ iteration " << it << ": ";
					std::cout << obj << std::endl;
					std::cout << "Random Exchange Local Search - Statistics @ iteration " << it << ": ";
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
	#if TRACE_RANDOM_EXCHANGE_LS >= 1
	std::cout << "Random Exchange Local Search - Statistics: ";
	std::cout << "feasible ratio = ";
	std::cout << static_cast<double>(feasibleCount) / static_cast<double>(evalCount) << ", ";
	std::cout << "commit ratio = ";
	std::cout << static_cast<double>(commitCount) / static_cast<double>(evalCount) << ", ";
	std::cout << "evaluations = " << evalCount << std::endl;
	#endif
	pool().push(info.objective(), info.solution());
	signalCompletion();
}
