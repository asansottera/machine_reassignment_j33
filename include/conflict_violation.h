#ifndef R12_CONFLICT_VIOLATION_H
#define R12_CONFLICT_VIOLATION_H

#include "common.h"
#include <functional>

namespace R12 {

class ConflictViolation {
private:
	ServiceID m_s;
	MachineID m_m;
public:
	ConflictViolation(ServiceID s, MachineID m)
	: m_s(s), m_m(m) {
	}
	ServiceID s() const { return m_s; }
	MachineID m() const { return m_m; }
	bool operator<(const ConflictViolation & v) const {
		if (&v == this) return false;
		if (s() == v.s()) return m() < v.m();
		return s() < v.s();
	}
	bool operator==(const ConflictViolation & v) const {
		if (&v == this) return true;
		return v.m() == m() && v.s() == s();
	}
};

struct ConflictViolationHash {
	std::size_t operator()(const ConflictViolation & v) const {
		return std::hash<ServiceID>()(v.s()) ^ std::hash<MachineID>()(v.m());
	}
};

}

#endif
