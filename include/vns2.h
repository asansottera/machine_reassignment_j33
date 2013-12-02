#ifndef R12_VNS2_H
#define R12_VNS2_H

#include "common.h"
#include "heuristic.h"
#include "move.h"
#include "exchange.h"
#include "advisor.h"
#include <vector>
#include <memory>
#include <boost/cstdint.hpp>
#include <boost/random.hpp>

namespace R12 {

class VNS2 : public Heuristic {
private:
	typedef boost::random::uniform_smallint<uint8_t> MethodDist;
	typedef boost::random::uniform_smallint<ProcessID> ProcessDist;
	typedef boost::random::uniform_smallint<MachineID> MachineDist;
private:
	std::unique_ptr<SolutionInfo> m_best;
	std::unique_ptr<SolutionInfo> m_current;
	boost::mt19937 m_rng;
	ProcessDist m_pDist;
	MachineDist m_mDist;
	std::unique_ptr<Advisor> m_advisor;
private: // options
	bool m_useAdvisor;
	bool m_useDynamicAdvisor;
	uint64_t m_kMin;
	uint64_t m_kMax;
	uint64_t m_maxTrialsLS;
	uint64_t m_maxTrialsShake;
private: // statistics
	uint64_t m_moveFeasibleEvalCount;
	uint64_t m_moveObjectiveEvalCount;
	uint64_t m_moveCommitCount;
	uint64_t m_moveEvalLocalSearch;
	uint64_t m_moveEvalShake;
	uint64_t m_moveCommitLocalSearch;
	uint64_t m_moveCommitShake;
	uint64_t m_exchangeFeasibleEvalCount;
	uint64_t m_exchangeObjectiveEvalCount;
	uint64_t m_exchangeCommitCount;
	uint64_t m_exchangeEvalLocalSearch;
	uint64_t m_exchangeEvalShake;
	uint64_t m_exchangeCommitLocalSearch;
	uint64_t m_exchangeCommitShake;
private:
	double moveCommitRatioLocalSearch() const {
		double eval = static_cast<double>(m_moveEvalLocalSearch);
		double commit = static_cast<double>(m_moveCommitLocalSearch);
		return commit / eval;
	}
	double moveCommitRatioShake() const {
		double eval = static_cast<double>(m_moveEvalShake);
		double commit = static_cast<double>(m_moveCommitShake);
		return commit / eval;
	}
	double exchangeCommitRatioLocalSearch() const {
		double eval = static_cast<double>(m_exchangeEvalLocalSearch);
		double commit = static_cast<double>(m_exchangeCommitLocalSearch);
		return commit / eval;
	}
	double exchangeCommitRatioShake() const {
		double eval = static_cast<double>(m_exchangeEvalShake);
		double commit = static_cast<double>(m_exchangeCommitShake);
		return commit / eval;
	}
private:
	Move randomMove();
	Exchange randomExchange();
	uint64_t defaultMaxTrialsLS();
	void localSearch();
	void shake(uint64_t k);
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
