#ifndef R12_ADVISOR_H
#define R12_ADVISOR_H

#include "problem.h"
#include "move.h"
#include "exchange.h"
#include "compatibility.h"
#include <vector>
#include <boost/random.hpp>

namespace R12 {

class Advisor {
private:
	Compatibility m_comp;
	boost::mt19937 & m_rng;
	boost::random::discrete_distribution<ProcessID> m_pDist;
public:
	Advisor(const Problem & instance,
			const std::vector<MachineID> & initial,
			boost::mt19937 & rng)
			: m_comp(instance, initial),
			  m_rng(rng) {
		ProcessCount pCount = instance.processes().size();
		std::vector<double> weights(pCount);
		for (ProcessCount pIdx = 0; pIdx < pCount; ++pIdx) {
			ProcessID p = m_comp.processByCompatibleCount(pIdx);
			if (m_comp.compatibleCount(p) == 1) {
				weights[p] = 0;
			} else {
				weights[p] = 1.0 + static_cast<double>(pIdx) / pCount;
			}
		}
		m_pDist = boost::random::discrete_distribution<ProcessCount>(weights);
	}
	Move adviseMove(const SolutionInfo & info) const {
		// select process
		ProcessID p;
		MachineID src;
		MachineCount mCompCount;
		do {
			p = m_pDist(m_rng);
			src = info.solution()[p];
			mCompCount = m_comp.compatibleCount(p);
		} while (mCompCount == 1);
		// select destination
		MachineID dst = src;
		auto dstIdxDist = boost::random::uniform_int_distribution<MachineCount>(0, mCompCount - 1);
		while (dst == src) {
			MachineCount dstIdx = dstIdxDist(m_rng);
			dst = m_comp.getCompatible(p, dstIdx);
		}
		return Move(p, src, dst);
	}
	Exchange adviseExchange(const SolutionInfo & info) const {
		ProcessID p1, p2;
		MachineID m1, m2;
		do {
			// select processes
			p1 = m_pDist(m_rng);
			p2 = m_pDist(m_rng);
			m1 = info.solution()[p1];
			m2 = info.solution()[p2];
		} while (p1 == p2 || !m_comp.compatible(p1, m2) || !m_comp.compatible(p2, m1));
		return Exchange(m1,p1,m2,p2);
	}
};

}

#endif
