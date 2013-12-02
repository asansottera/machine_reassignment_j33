#ifndef R12_COMPATIBILITY_H
#define R12_COMPATIBILITY_H

#include "common.h"
#include "problem.h"
#include "solution_info.h"
#include <vector>
#include <algorithm>

namespace R12 {

class Compatibility {
private:
	std::vector<bool> m_compMatrix;
	std::vector<std::vector<MachineID>> m_comp;
	std::vector<ProcessID> m_pByCompCount;
	uint32_t m_compCount;
private:
	class CompatibleCountCompare {
	private:
		const Compatibility & m_compatibility;
	public:
		CompatibleCountCompare(const Compatibility & compatibility)
		: m_compatibility(compatibility) {
		}
		bool operator()(const ProcessID p1, const ProcessID p2) const {
			return m_compatibility.compatibleCount(p1) < m_compatibility.compatibleCount(p2);
		}
	};
public:
	Compatibility(const Problem & instance, const std::vector<MachineID> & initial) {
		const ProcessCount pCount = instance.processes().size();
		const MachineCount mCount = instance.machines().size();
		// initialize compatibility
		m_comp.resize(pCount);
		m_compMatrix.resize(pCount * mCount);
		SolutionInfo initialInfo(instance, initial);
		m_compCount = 0;
		for (MachineID m = 0; m < mCount; ++m) {
			const Machine & machine = instance.machines()[m];
			// compute usable capacity at machine m
			std::vector<uint32_t> usable(instance.resources().size());
			const std::vector<ResourceID> & transient = instance.transientResources();
			for (auto itr = transient.begin(); itr != transient.end(); ++itr) {
				const ResourceID r = *itr;
				usable[r] = machine.capacity(r) - initialInfo.usage(m, r);
			}
			const std::vector<ResourceID> & nonTransient = instance.nonTransientResources();
			for (auto itr = nonTransient.begin(); itr != nonTransient.end(); ++itr) {
				const ResourceID r = *itr;
				usable[r] = machine.capacity(r);
			}
			// determine compatible processes
			for (ProcessID p = 0; p < pCount; ++p) {
				const Process & process = instance.processes()[p];
				bool compatible = true;
				if (initial[p] != m) {
					for (ResourceID r = 0; r < instance.resources().size(); ++r) {
						if (process.requirement(r) > usable[r]) {
							compatible = false;
							break;
						}
					}
				}
				if (compatible) {
					m_comp[p].push_back(m);
					m_compMatrix[p + m * pCount] = true;
					++m_compCount;
				}
			}
		}
		// sort compatible machines
		for (ProcessID p = 0; p < instance.processes().size(); ++p) {
			CHECK(m_comp[p].size() >= 1);
			std::sort(m_comp[p].begin(), m_comp[p].end());
		}
		// processes sorted by compatibility
		m_pByCompCount.resize(instance.processes().size());
		for (ProcessID p = 0; p < instance.processes().size(); ++p) {
			m_pByCompCount[p] = p;
		}
		std::sort(m_pByCompCount.begin(), m_pByCompCount.end(), CompatibleCountCompare(*this));
	}
	bool compatible(const ProcessID p, const MachineID m) const {
		ProcessCount pCount = m_comp.size();
		return m_compMatrix[p + m * pCount];
	}
	MachineCount compatibleCount(const ProcessID p) const {
		return m_comp[p].size();
	}
	MachineID getCompatible(const ProcessID p, const MachineCount idx) const {
		return m_comp[p][idx];
	}
	ProcessID processByCompatibleCount(const ProcessCount idx) const {
		return m_pByCompCount[idx];
	}
	uint32_t totalCompatibleCount() const {
		return m_compCount;
	}
};

}

#endif
