#ifndef R12_VNS3_H
#define R12_VNS3_H

#include "common.h"
#include "heuristic.h"
#include "move.h"
#include "exchange.h"
#include <vector>
#include <memory>
#include <boost/cstdint.hpp>
#include <boost/random.hpp>

namespace R12 {

class VNS3 : public Heuristic {
private:
	std::unique_ptr<SolutionInfo> m_best;
	std::unique_ptr<SolutionInfo> m_current;
	boost::mt19937 m_rng;
private: // parameters
	uint64_t m_kMin;
	uint64_t m_kMax;
	uint64_t m_kStep;
public:
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
