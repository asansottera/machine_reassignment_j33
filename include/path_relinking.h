#ifndef R12_PATH_RELINKING_H
#define R12_PATH_RELINKING_H

#include "common.h"
#include "heuristic.h"
#include "move.h"
#include "solution_pool.h"
#include "solution_info.h"
#include <vector>
#include <list>

namespace R12 {

class PathRelinking : public Heuristic {
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
private:
	bool step(SolutionInfo & info1, const SolutionInfo & v2, std::list<ProcessID> & differences);
	void relink(const SolutionPool::Entry s1, const SolutionPool::Entry s2);
};

}

#endif
