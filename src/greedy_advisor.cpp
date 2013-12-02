#include "greedy_advisor.h"
#include <algorithm>

using namespace R12;

class _GapComparer {
private:
	const std::vector<int64_t> & m_gaps;
public:
	_GapComparer(const std::vector<int64_t> & gaps)
		: m_gaps(gaps) {
	}
	bool operator()(MachineID a, MachineID b) const {
		return m_gaps[a] < m_gaps[b];
	}
};

GreedyAdvisor::GreedyAdvisor(const SolutionInfo & info, const std::vector<std::set<ProcessID>> & processesByMachine)
: m_info(info), m_processesByMachine(processesByMachine) {
	initGaps();
	initDistribution();
	m_moveCounter = 0;
	m_refreshPeriod = 10;
}

void GreedyAdvisor::initGaps() {
	std::size_t mCount = m_info.instance().machines().size();
	std::size_t rCount = m_info.instance().resources().size();
	// compute gaps
	m_gaps.resize(mCount);
	for (MachineID m = 0; m < mCount; ++m) {
		const Machine & machine = m_info.instance().machines()[m];
		int64_t gap = 0;
		for (ResourceID r = 0; r < rCount; ++r) {
			const Resource & resource = m_info.instance().resources()[r];
			int64_t rDelta = static_cast<int64_t>(m_info.usage(m, r)) - static_cast<int64_t>(machine.safetyCapacity(r));
			gap += resource.weightLoadCost() * rDelta;
		}
		m_gaps[m] = gap;
	}
	// build sorted list of machines
	m_machinesByGap.resize(mCount);
	for (MachineID m = 0; m < mCount; ++m) {
		m_machinesByGap[m] = m;
	}
	std::sort(m_machinesByGap.begin(), m_machinesByGap.end(), _GapComparer(m_gaps));
}

void GreedyAdvisor::initDistribution() {
	std::size_t mCount = m_info.instance().machines().size();
	std::vector<double> weights(mCount, 0);
	for (MachineID m = 0; m < mCount; ++m) {
		weights[m] = 1.0 - (1.0 / mCount);
	}
	m_distribution = boost::random::discrete_distribution<MachineID>(weights.begin(), weights.end());
}

void GreedyAdvisor::refresh() {
	std::sort(m_machinesByGap.begin(), m_machinesByGap.end(), _GapComparer(m_gaps));
}

Move GreedyAdvisor::advise() {
	std::size_t mCount = m_info.instance().machines().size();
	// pick with high probability machine with large gap
	MachineID src;
	std::size_t pCountOnSrc;
	do {
		src = static_cast<MachineID>(mCount - 1) - m_distribution(m_gen);
		pCountOnSrc = m_processesByMachine[src].size();
	} while (pCountOnSrc == 0);
	// pick with high probability machine with small gap
	MachineID dst = m_distribution(m_gen);
	// pick a random process on the source machine
	boost::random::uniform_int_distribution<std::size_t> pIndexDistribution(0, pCountOnSrc - 1);
	std::size_t pIndex = pIndexDistribution(m_gen);
	auto itr = m_processesByMachine[src].begin();
	std::advance(itr, pIndex);
	ProcessID p = *itr;
	// return proposed move
	return Move(p, src, dst);
}

void GreedyAdvisor::notify(const Move & move) {
	// update gaps
	std::size_t rCount = m_info.instance().resources().size();
	const Process & process = m_info.instance().processes()[move.p()];
	for (ResourceID r = 0; r < rCount; ++r) {
		const Resource & resource = m_info.instance().resources()[r];
		m_gaps[move.src()] -= resource.weightLoadCost() * process.requirement(r);
		m_gaps[move.dst()] += resource.weightLoadCost() * process.requirement(r);
	}
	// increase move counter and rebuild ditributions
	++m_moveCounter;
	if (m_moveCounter % m_refreshPeriod == 0) {
		refresh();
	}
}
