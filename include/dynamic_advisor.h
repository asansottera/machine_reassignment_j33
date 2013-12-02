#ifndef R12_DYNAMIC_ADVISOR_H
#define R12_DYNAMIC_ADVISOR_H

#include "common.h"
#include "solution_info.h"
#include "move.h"
#include "exchange.h"
#include <vector>
#include <boost/random.hpp>

namespace R12 {

class DynamicAdvisor {
private:
	const SolutionInfo & m_info;
	std::vector<bool> m_highLoad;
	std::vector<MachineID> m_lowLoadMachines;
	std::vector<MachineID> m_highLoadMachines;
	std::vector<std::vector<ProcessID>> m_pm;
	mutable boost::mt19937 m_rng;
	boost::random::uniform_smallint<MachineID> m_mDist;
	boost::random::uniform_smallint<MachineCount> m_idxHighLoadDist;
	boost::random::uniform_smallint<MachineCount> m_idxLowLoadDist;
	std::vector<boost::random::uniform_smallint<ProcessCount>> m_pmDist;
private:
	bool checkHighLoad(const MachineID m) const {
		const Machine & machine = m_info.instance().machines()[m];
		bool highLoad = false;
		for (ResourceID r = 0; r < m_info.instance().resources().size(); ++r) {
			if (m_info.usage(m, r) > machine.safetyCapacity(r)) {
				highLoad = true;
				break;
			}
		}
		return highLoad;
	}
	void markHighLoad(const MachineID dst) {
		m_highLoad[dst] = true;
		auto eraseItr = std::equal_range(m_lowLoadMachines.begin(), m_lowLoadMachines.end(), dst).first;
		m_lowLoadMachines.erase(eraseItr);
		auto insertItr = std::lower_bound(m_highLoadMachines.begin(), m_highLoadMachines.end(), dst);
		m_highLoadMachines.insert(insertItr, dst);
	}
	void markLowLoad(const MachineID src) {
		m_highLoad[src] = false;
		auto eraseItr = std::equal_range(m_highLoadMachines.begin(), m_highLoadMachines.end(), src).first;
		m_highLoadMachines.erase(eraseItr);
		auto insertItr = std::lower_bound(m_lowLoadMachines.begin(), m_lowLoadMachines.end(), src);
		m_lowLoadMachines.insert(insertItr, src);
	}
	void initLowLoadDist() {
		if (m_highLoadMachines.size() > 0) {
			m_idxHighLoadDist = boost::random::uniform_smallint<MachineCount>(0, m_highLoadMachines.size() - 1);
		}
	}
	void initHighLoadDist() {
		if (m_lowLoadMachines.size() > 0) {
			m_idxLowLoadDist = boost::random::uniform_smallint<MachineCount>(0, m_lowLoadMachines.size() - 1);
		}
	}
	void initProcessOnMachineDist(const MachineID m) {
		m_pmDist[m] = boost::random::uniform_smallint<ProcessCount>(0, m_pm[m].size() - 1);	
	}
	void moveProcess(const ProcessID p, const MachineID src, const MachineID dst) {
		auto eraseItr = std::equal_range(m_pm[src].begin(), m_pm[src].end(), p).first;
		m_pm[src].erase(eraseItr);
		auto insertItr = std::lower_bound(m_pm[dst].begin(), m_pm[dst].end(), p);
		m_pm[dst].insert(insertItr, p);
		initProcessOnMachineDist(src);
		initProcessOnMachineDist(dst);
	}
public:
	DynamicAdvisor(const SolutionInfo & info, const uint32_t seed) : m_info(info), m_rng(seed) {
		m_highLoad.resize(m_info.instance().machines().size());
		for (MachineID m = 0; m < m_info.instance().machines().size(); ++m) {
			m_highLoad[m] = checkHighLoad(m);
			if (m_highLoad[m]) {
				m_highLoadMachines.push_back(m);
			} else {
				m_lowLoadMachines.push_back(m);
			}
		}
		std::sort(m_highLoadMachines.begin(), m_highLoadMachines.end());
		std::sort(m_lowLoadMachines.begin(), m_lowLoadMachines.end());
		m_mDist = boost::random::uniform_smallint<MachineID>(0, m_info.instance().machines().size() - 1);
		initLowLoadDist();
		initHighLoadDist();
		m_pm.resize(m_info.instance().processes().size());
		m_pmDist.resize(m_info.instance().processes().size());
		for (ProcessID p = 0; p < m_info.instance().processes().size(); ++p) {
			MachineID m = m_info.solution()[p];
			m_pm[m].push_back(p);
		}
		for (MachineID m = 0; m < m_info.instance().machines().size(); ++m) {
			std::sort(m_pm[m].begin(), m_pm[m].end());
			initProcessOnMachineDist(m);
		}
	}
	Move adviseMove() const {
		ProcessID p;
		MachineID src, dst;
		if (m_lowLoadMachines.size() > 0 && m_highLoadMachines.size() > 0) {
			src = m_highLoadMachines[m_idxHighLoadDist(m_rng)];
			p = m_pm[src][m_pmDist[src](m_rng)];
			dst = m_lowLoadMachines[m_idxLowLoadDist(m_rng)];
			return Move(p, src, dst);
		} else  {
			// either no low load machine (unlucky)
			// or no high load machines (lucky)
			do {
				src = m_mDist(m_rng);
				dst = m_mDist(m_rng);
			} while (src != dst);
			p = m_pm[src][m_pmDist[src](m_rng)];
		}
		return Move(p, src, dst);
	}
	// TODO Exchange adviseExchange() const;
	void notify(const Move & move) {
		const MachineID src = move.src();
		const MachineID dst = move.dst();
		bool changed = false;
		if (!m_highLoad[dst] && checkHighLoad(dst)) {
			markHighLoad(dst);
			changed = true;
		}
		if (m_highLoad[src] && !checkHighLoad(src)) {
			markLowLoad(src);
			changed = true;
		}
		if (changed) {
			initHighLoadDist();
			initLowLoadDist();
		}
		moveProcess(move.p(), move.src(), move.dst());
	}
	void notify(const Exchange & exchange) {
		const MachineID m1 = exchange.m1();
		const MachineID m2 = exchange.m2();
		bool highLoad1 = checkHighLoad(m1);
		bool highLoad2 = checkHighLoad(m2);
		bool changed = false;
		if (!m_highLoad[m1] && highLoad1) {
			markHighLoad(m1);
			changed = true;
		} else if (m_highLoad[m1] && !highLoad1) {
			markLowLoad(m1);
			changed = true;
		}
		if (!m_highLoad[m2] && highLoad2) {
			markHighLoad(m2);
			changed = true;
		} else if (m_highLoad[m2] && !highLoad2) {
			markLowLoad(m2);
			changed = true;
		}
		if (changed) {
			initHighLoadDist();
			initLowLoadDist();
		}
		moveProcess(exchange.p1(), m1, m2);
		moveProcess(exchange.p2(), m2, m1);
	}
};

}

#endif
