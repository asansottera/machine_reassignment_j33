#ifndef R12_GREEDY_ADVISOR_H
#define R12_GREEDY_ADVISOR_H

#include "move.h"
#include "solution_info.h"
#include <vector>
#include <set>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/discrete_distribution.hpp>

namespace R12 {

class GreedyAdvisor {
private: // settings
	uint32_t m_refreshPeriod;
private: // references to shared objects
	const SolutionInfo & m_info;
	const std::vector<std::set<ProcessID>> & m_processesByMachine;
private:
	std::vector<int64_t> m_gaps;
	boost::mt19937 m_gen;
	std::vector<MachineID> m_machinesByGap;
	boost::random::discrete_distribution<MachineID> m_distribution;
	uint64_t m_moveCounter;
private:
	void initGaps();
	void initDistribution();
	void refresh();
public:
	GreedyAdvisor(const SolutionInfo & info, const std::vector<std::set<ProcessID>> & processesByMachine);
	uint32_t refreshPeriod() const { return m_refreshPeriod; }
	void setRefreshPeriod(uint32_t refreshPeriod) { m_refreshPeriod = refreshPeriod; }
	Move advise();
	void notify(const Move & move);
};

}

#endif
