#ifndef R12_MULTI_START_LOCAL_SEARCH_H
#define R12_MULTI_START_LOCAL_SEARCH_H

#include "heuristic.h"

namespace R12 {

class MultiStartLocalSearch : public Heuristic {
private:
	uint64_t m_bestObjective;
	std::vector<MachineID> m_bestSolution;
public:
	virtual void run();
	virtual uint64_t bestObjective() const {
		return m_bestObjective;
	}
	virtual const std::vector<MachineID> & bestSolution() const {
		return m_bestSolution;
	}
};

}

#endif
