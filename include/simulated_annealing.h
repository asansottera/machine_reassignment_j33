#ifndef R12_SIMULATED_ANNEALING_H
#define R12_SIMULATED_ANNEALING_H

#include "common.h"
#include "heuristic.h"
#include "problem.h"
#include "move_verifier.h"
#include "exchange_verifier.h"
#include <boost/random.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <math.h>

//#define TRACE_SA 0

namespace R12 {

	// Parameter to reduce temperature
	const static long double RedPar = 0.97; 
	// Max number of unfeasible moves to reset temperature
	const static uint64_t MaxNMov = 500;

class SimulatedAnnealing : public Heuristic
{
private:

	std::unique_ptr<SolutionInfo> m_bestSolution;

	#ifdef TRACE_SA
	uint64_t moveCount;
	#endif

	boost::mt19937 m_rng;
	ParameterMap SAlsParameters;
	uint32_t iteration;

	#ifdef TRACE_SA
	void printTemperature () { std::cout << "Temperature at iteration " << iteration << " equal to: " << Temperature << std::endl;}
	#endif

	long double Temperature;
	long double MinTemperature;
	long double MaxTemperature;
	long double ReduceParameterTemperature;
	bool ReadPool;
	
	inline void configure();
	inline void initTemperature () { Temperature = MaxTemperature;}
	inline void reduceTemperature() { Temperature *= ReduceParameterTemperature;}
	inline bool StopCondition() { return interrupted();}

	inline void setMaxTemperature(long double Temp) { MaxTemperature = Temp;}
	inline void setReduceTemperature(long double RPar) { ReduceParameterTemperature = RPar; }
	inline void setMinTemperature(long double Temp) { MinTemperature = Temp;}
	inline void setReadPool(bool readPool){ ReadPool = readPool;}
	void Init();
	
public:

	void runFromSolution(SolutionInfo & info);
	virtual void run();
	virtual const std::vector<MachineID> & bestSolution() const { return m_bestSolution->solution(); }
	virtual uint64_t bestObjective() const { return m_bestSolution->objective(); }
};

}

#endif
