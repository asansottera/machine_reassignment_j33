#include "SA_local_search_routine.h"
#include "SA_constants.h"

using namespace R12;

#ifdef CHECK_SALS
Verifier::Result SALocalSearchRoutine::printFeasibleError(Move & move, bool feasible, SolutionInfo & info)
{
	
	std::vector<MachineID> newSolution = info.solution();
	newSolution[move.p()] = move.dst();
	Verifier::Result result = SALocalSearchRoutine::verifier.verify(instance(), initial(), newSolution);
	if (feasible != result.feasible()) {
			std::cout << "Simulated annealing local search - Wrong feasibility at iteration " << iteration << std::endl;
			throw std::runtime_error("Simulated annealing local search - Wrong feasibility");
		}
	return result;
}

void SALocalSearchRoutine::printCostFunctionError(uint64_t objective, Verifier::Result result)
{
	if (objective != result.objective()) {
		std::cout << "Simulated annealing - Wrong objective value " << objective << " respect to " << result.objective() <<  " at iteration " << iteration << std::endl;
							 
		throw std::runtime_error("Simulated annealing - Wrong objective");
	}
	
}
#endif

/*
Move SALocalSearchRoutine::randomMove(const SolutionInfo & info) {
	ProcessID p;
	MachineID src;
	MachineID dst;
	do {
		p =  m_pDist(rng());
		src = info.solution()[p];
		dst =  m_mDist(rng());
	} while (src == dst);
	Move move(p, src, dst);
	return move;
}

Exchange SALocalSearchRoutine::randomExchange(const SolutionInfo & info) {
	ProcessID p1, p2;
	do {
		p1 = m_pDist(rng());
		p2 = m_pDist(rng());
	} while (p1 == p2);
	MachineID m1 = info.solution()[p1];
	MachineID m2 = info.solution()[p2];
	Exchange exchange(m1, p1, m2, p2);
	return exchange;
}
*/

void SALocalSearchRoutine::configure(const ParameterMap & parameters) {
	m_pDist = ProcessDist(0, instance().processes().size() - 1);
	m_mDist = MachineDist(0, instance().machines().size() - 1);

	MaxIterations = ceil(instance().processes().size()*(log10(static_cast<long double> (instance().processes().size()))+log10(static_cast<long double> (instance().machines().size()))));
	
	MinTemperature = parameters.param<long double>("min_t", MinTemp);
	IProbMove = parameters.param<long double>("i_prob", InitProb);
	MinProbMove = parameters.param<long double>("min_prob", MinProb);
}


long double SALocalSearchRoutine::Probability(long double MaxT, long double T)
{
	long double Prob = (IProbMove * (log(T) - log(MinTemperature))/(log(MaxT) - log(MinTemperature)));
	if(Prob<MinProbMove)
		return MinProbMove;
	return Prob;
}


void SALocalSearchRoutine::SA_search(SolutionInfo & info, uint64_t bestObjective, long double MaxT, long double T)
{
		enum Op { MoveOp, ExchangeOp };
		ExchangeVerifier exverifier(info);
		MoveVerifier mverifier(info);
		SolutionInfo InitSolution(instance(),initial());
		uint64_t InitObjective = InitSolution.objective();
			
		BestMoveTemperature = -1;
		bool feasible=false;
		long double prob_move = IProbMove;

		boost::uniform_01<boost::mt19937,long double> dist_delta(rng());
		boost::uniform_01<boost::mt19937,long double> method(rng());
		Op op  = MoveOp;
		Move move(0,0,0);
		Exchange exmove(0,0,0,0);
			
		ProcessID p = m_pDist(rng());	
		MachineID dst = m_mDist(rng());	
		ProcessID p1 = m_pDist(rng());
		ProcessID p2;

		do
		{
			p2 = m_pDist(rng());
		} while (p1==p2);
	
		iteration = 0;
		long double ActualProb = Probability(MaxT, T);

		#ifdef TRACE_SALS
		//std::cout << "Probability to make a move: " << ActualProb << std::endl;
		#endif

		while (!IterationEnd(iteration))
		{
			++iteration;
			feasible = false;
			prob_move = method();
		
			if(prob_move  < ActualProb )
			{
				op = MoveOp;
				p = (p+1)  % instance().processes().size();
				MachineID src = info.solution()[p];

				do
				{
					dst = (dst+1)  % instance().machines().size();
				}
				while(src==dst);
		
				move = Move(p,src,dst);
				feasible = mverifier.feasible(move);		
			}
			else
			{		
				op = ExchangeOp;		
				
				p1 = (p1 + 1)  % instance().processes().size();
				MachineID m1 = info.solution()[p1];
				MachineID m2;
				do
				{
					p2 = (p2 + 1) % instance().processes().size();
					m2 = info.solution()[p2];
				}
				while (m1==m2);

				exmove = Exchange(m1,p1,m2,p2);
				feasible = exverifier.feasible(exmove);
				
			}

			#ifdef CHECK_SALS
			Verifier::Result result = printFeasibleError(move,feasible,info);
			#endif

			if (feasible) 
			{
				int64_t new_objective;
				if (op == MoveOp)
					new_objective = mverifier.objective(move);
				else
					new_objective = exverifier.objective(exmove);

				int64_t diffobjective = new_objective  - bestObjective;
				if (diffobjective < 0) 
				{	
					BestMoveTemperature = T;

					bestObjective = new_objective;

					if (op == MoveOp)
						mverifier.commit(move);
					else
						exverifier.commit(exmove);
			
				}	
				else
				{	
					
					//if(new_objective<=InitObjective)
					{
						long double delta = dist_delta();
						if(delta < exp(-(static_cast<long double>(diffobjective)/T)))
						{
							bestObjective = new_objective;
							if (op == MoveOp)
								mverifier.commit(move);
							else
								exverifier.commit(exmove);
						}
					}
				}
				#ifdef CHECK_SALS
				printCostFunctionError(new_objective,result);
				#endif
			}

			
		}// end process loop	
}