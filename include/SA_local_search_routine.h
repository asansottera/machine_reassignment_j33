#ifndef R12_SA_LOCAL_SEARCH_ROUTINE_H
#define R12_SA_LOCAL_SEARCH_ROUTINE_H

//#define CHECK_SALS 1
//#define TRACE_SALS 0

#include "verifier.h"
#include "common.h"
#include "problem.h"
#include "solution_info.h"
#include "move.h"
#include "exchange.h"
#include "atomic_flag.h"
#include "parameter_map.h"
#include "SA_local_search.h"
#include "move_verifier.h"
#include "exchange_verifier.h"

#include <vector>
#include <boost/cstdint.hpp>
#include <boost/random.hpp>

namespace R12 {


	// Max probability to take a move instead of an exchange
	const static long double InitProb = 0.8;
	// Min probability to take a move instead of an exchange
	const static long double MinProb = 0.2;

class SALocalSearchRoutine : public SALocalSearch
{
	private:
		typedef boost::random::uniform_smallint<ProcessID> ProcessDist;
		typedef boost::random::uniform_smallint<MachineID> MachineDist;

		long double MinTemperature;
		long double IProbMove;
		long double MinProbMove;
		uint32_t MaxIterations;
		uint64_t iteration;

		#ifdef CHECK_SALS
		Verifier verifier;
		Verifier::Result printFeasibleError (Move & move, bool feasible, SolutionInfo & info);
		void printCostFunctionError(uint64_t objective, Verifier::Result result);
		#endif

		// constructed during initialization
		ProcessDist m_pDist;
		MachineDist m_mDist;

		inline bool IterationEnd(uint32_t iteration) { return (iteration>MaxIterations || interrupted());}

		// Statistics
		long double BestMoveTemperature;
		uint32_t NullMoves;
		uint64_t IntialObjective;

		//Move randomMove(const SolutionInfo & info);
		//Exchange randomExchange(const SolutionInfo & info);
		long double Probability(long double MaxT, long double T);

	public:
		virtual void configure(const ParameterMap & parameters);
		virtual void SA_search(SolutionInfo & info, uint64_t bestObjective, long double MaxT, long double T);
		const uint32_t getNullMoves() const {return NullMoves;}
		const long double getBestMoveTemperature() const {return BestMoveTemperature;}
};

}

#endif
