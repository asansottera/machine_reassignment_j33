#ifndef R12_ELS_H
#define R12_ELS_H

#include "heuristic.h"

namespace R12 {

class ELS : public Heuristic {
private:
	uint64_t m_bestObjective;
	std::vector<MachineID> m_bestSolution;
public:
	int64_t distance(const SolutionInfo & info, const MachineID m) const;
	int64_t distance(const SolutionInfo & info, const MachineID m, const ProcessID pOut, const ProcessID pIn) const; 
	virtual void run() {
		SolutionInfo info(instance(), initial());
		runFromSolution(info);
		pool().push(bestObjective(), bestSolution());
		signalCompletion();
	}
	virtual void runFromSolution(SolutionInfo & info);
	virtual uint64_t bestObjective() const {
		return m_bestObjective;
	}
	virtual const std::vector<MachineID> & bestSolution() const {
		return m_bestSolution;
	}
};

}

#endif
