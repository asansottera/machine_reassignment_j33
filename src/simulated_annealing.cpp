#include "simulated_annealing.h"
#include "SA_constants.h"

#include "SA_local_search_routine.h"
#include <iostream>
#include <boost/random/uniform_01.hpp>
#include "batch_verifier.h"
#include <limits>

using namespace R12;



inline void SimulatedAnnealing::configure()
{
	m_rng.seed(seed());
	
	setMinTemperature(param<long double>("min_t",MinTemp));
	setReduceTemperature(param<long double>("r_factor",RedPar));
	setReadPool(param<bool>("read_pool",false));

	std::string SAlsName = param<std::string>("SAls");
	SAlsParameters = parameters().extractGroup("SAls");
}

void SimulatedAnnealing::run()
{
	configure();	
	m_bestSolution.reset(new SolutionInfo(instance(), initial()));

	uint64_t problemSizeLimit = 50000 * 1000;
	if(instance().processes().size() * instance().machines().size() > problemSizeLimit){
		// big instance, don't use SA
		signalCompletion();
		return;
	}
	
	Init();
	
	while(!interrupted())
	{
		runFromSolution(*m_bestSolution);
		SolutionPool::Entry best;
		pool().best(best);	
		m_bestSolution.reset(new SolutionInfo(instance(), initial(), *(best.ptr()))); 		
	}

#ifdef TRACE_SA
	std::cout << "Simulated annealing - Ends Best solution: " << m_bestSolution->objective() << std::endl;
#endif

	signalCompletion();
}


void SimulatedAnnealing::runFromSolution(SolutionInfo & info) 
{

	std::unique_ptr<SALocalSearchRoutine> SAls(new SALocalSearchRoutine);

	SAls->init(instance(), initial(), flag(), m_rng, SAlsParameters);

	int64_t bestObjective = info.objective();
	int64_t lastObjective ;
	
	#ifdef TRACE_SA
	moveCount = 0;
	#endif

	//std::vector<MachineID> initial_solution = info.solution();

	pool().push(bestObjective,info.solution());
	
	initTemperature();

	long double ResetTemperature = Temperature;
	long double BestMoveTemperature = Temperature;
	
	uint32_t MAX_DIVISIONS = 100;
	uint32_t MAX_DIVISIONS_RESET = 50;
	
	long double LowestTemperature = Temperature / pow(2, MAX_DIVISIONS);
	long double LowestResetTemperature = Temperature / pow(2, MAX_DIVISIONS_RESET);
	
	// ***
	// start simulated annealing search
	// ***
	const double THRESHOLD = 1;
	uint32_t violationCounter = 0;
	uint32_t VIOLATION_THRESHOLD = 15;
		
	uint64_t counter = 0;
		
	while (!StopCondition()) 
	{

		bestObjective = m_bestSolution->objective();
		
		SAls->SA_search(*m_bestSolution, m_bestSolution -> objective(),MaxTemperature,Temperature);

		lastObjective = m_bestSolution->objective();
		
		// If getBestMoveTemperature() returns -1, than no moves have been executed
		if(SAls->getBestMoveTemperature()>=0)
			BestMoveTemperature = SAls->getBestMoveTemperature();

		pool().push(m_bestSolution->objective(), m_bestSolution->solution());

		double ratio = (double)lastObjective/bestObjective;

		if(ratio >= THRESHOLD){
			if(violationCounter < VIOLATION_THRESHOLD){
			 	++violationCounter;
			}
		}
		else {
			if(violationCounter > 0){
				--violationCounter;
			}
		}

		if(violationCounter >= VIOLATION_THRESHOLD)
		{
			violationCounter = 0;
			ResetTemperature /= 2;
			Temperature = (ResetTemperature >= Temperature) ? ResetTemperature : BestMoveTemperature;

			if(ResetTemperature <  LowestResetTemperature)
				ResetTemperature = Temperature / pow(2, MAX_DIVISIONS_RESET);
			
			if(ReadPool){
				SolutionPool::Entry entry;
				if (pool().best(entry)) {
					if (entry.obj() < m_bestSolution->objective()) {
						m_bestSolution.reset(new SolutionInfo(instance(), initial(), *(entry.ptr())));
					}
				}
			}

#ifdef TRACE_SA
			std::cout << "Resetting to temperature " << Temperature << std::endl;
#endif
		}
		else{
			if(Temperature >= LowestTemperature)
				reduceTemperature();
		}
		

		#ifdef TRACE_SA
		if(++counter % 50 == 0){
			std::cout<<"best solution " <<m_bestSolution->objective()<< std::endl;
			std::cout<<"ratio: "<< ratio << std::endl;
			printTemperature();
		}
		#endif
	} // end simulated annealing loop

	//m_bestSolution =  info.solution();

}

void SimulatedAnnealing::Init()
{
	ProcessCount pCount = instance().processes().size();
	MachineCount mCount = instance().machines().size();
	MoveVerifier mv(*m_bestSolution);
	ExchangeVerifier ev(*m_bestSolution);
	uint64_t MaxDiff = 0;

	// perform one iteration
	uint64_t samples = 0;
	ProcessID p = 0;
	MachineID dst = 0;	
	ProcessID p1 = 0;	
	ProcessID p2 = 0;
	uint64_t m_maxSamples = instance().processes().size() * instance().machines().size();
	uint64_t FeasibleSolutions = 0;

	while (samples < m_maxSamples)
	{
		samples++;
		// try move or exchange
		if (samples % 2 == 0)
		{
			// Move
			p = (p + 1) % pCount;
			dst = (dst + 1) % mCount;
			MachineID src =	(*m_bestSolution).solution()[p];
			if (src != dst)
			{
				Move move(p, src, dst);
				if (mv.feasible(move)) 
				{
					FeasibleSolutions++;
					
					uint64_t obj = mv.objective(move);
					uint64_t Diff = std::abs ((int64_t)obj - (int64_t)m_bestSolution->objective() );
			
					if(Diff > MaxDiff)
						MaxDiff = Diff;
				}
			}
		}
		else {
			// Exchange
			p1 = (p1 + 1) % pCount;
			do
			{
				p2 = (p2 + 1) % pCount;
			} while(p1 == p2) ;

			MachineID m1 = (*m_bestSolution).solution()[p1];
			MachineID m2 = (*m_bestSolution).solution()[p2];
			if (m1 != m2) 
	     	{
				Exchange exchange(m1, p1, m2, p2);
				if (ev.feasible(exchange)) 
				{
					FeasibleSolutions++;
					uint64_t obj = ev.objective(exchange);
					uint64_t Diff = std::abs ((int64_t)obj - (int64_t)m_bestSolution->objective() );
		
					if(Diff > MaxDiff)
						MaxDiff = Diff;
				}
			}
		}
	}
	
	if(FeasibleSolutions>0)
		setMaxTemperature(MaxDiff);
	else
		setMaxTemperature(m_bestSolution->objective());

#ifdef TRACE_SA
	std::cout << "Max temperature: " << MaxTemperature << " Min temperature: " << MinTemperature << std::endl;
#endif


}