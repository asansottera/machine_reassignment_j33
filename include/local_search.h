#ifndef R12_LOCAL_SEARCH_H
#define R12_LOCAL_SEARCH_H

#include "heuristic.h"
#include "problem.h"
#include "verifier.h"

namespace R12 {

/*! Performs a first improvement local search. */
class LocalSearch : public Heuristic {
private:
	Verifier m_verifier;
	std::vector<MachineID> m_bestSolution;
	uint32_t m_bestObjective;
private:
	/*! Copy construction is disabled. */
	LocalSearch(const LocalSearch & other) {
	}
	/*! Assignment operator is disabled */
	LocalSearch & operator=(const LocalSearch & other) {
		return *this;
	}
public:
	LocalSearch() {
		m_bestObjective = 0;
	}
	~LocalSearch() {
	}	
	virtual void run();
	virtual const std::vector<MachineID> & bestSolution() const { return m_bestSolution; }
	virtual uint64_t bestObjective() const { return m_bestObjective; }
};

}

#endif
