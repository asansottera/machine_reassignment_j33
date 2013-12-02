#ifndef R12_VNS_H
#define R12_VNS_H

#include "heuristic.h"
#include "solution_info.h"
#include <memory>
#include <boost/random.hpp>

namespace R12 {

class VNS : public Heuristic {
private:
	std::unique_ptr<SolutionInfo> m_current;
	std::unique_ptr<SolutionInfo> m_best;
private:
	boost::mt19937 m_rng;
private:
	uint64_t m_maxTrialsLS;
	uint64_t m_maxSamplesLS;
private:
	uint64_t defaultMaxTrialsLS();
	void localSearch();
public:
	VNS() {}
	virtual void run();
	virtual const std::vector<MachineID> & bestSolution() const {
		return m_best->solution();
	}
	virtual uint64_t bestObjective() const {
		return m_best->objective();
	}
};

}

#endif
