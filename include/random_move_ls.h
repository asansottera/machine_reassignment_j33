#ifndef R12_RANDOM_MOVE_LS
#define R12_RANDOM_MOVE_LS

#include "common.h"
#include "heuristic.h"
#include <vector>

namespace R12 {

class RandomMoveLS : public Heuristic {
private:
	std::vector<MachineID> m_bestSolution;
	uint64_t m_bestObjective;
public:
	virtual void run();
	virtual const std::vector<MachineID> & bestSolution() const { return m_bestSolution; }
	virtual uint64_t bestObjective() const { return m_bestObjective; }
};

}

#endif
