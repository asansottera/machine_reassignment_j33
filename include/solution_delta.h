#ifndef R12_SOLUTION_DELTA_H
#define R12_SOLUTION_DELTA_H

#include "common.h"
#include <vector>

namespace R12 {

inline ProcessCount delta(const std::vector<MachineID> & s1, const std::vector<MachineID> & s2) {
	CHECK(s1.size() == s2.size());
	ProcessCount pc = 0;
	for (ProcessID p = 0; p < s1.size(); ++p) {
		if (s1[p] != s2[p]) {
			++pc;
		}
	}
	return pc;
}

}

#endif
