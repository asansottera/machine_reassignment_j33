#ifndef R12_DEPENDENCY_VIOLATION_H
#define R12_DEPENDENCY_VIOLATION_H

#include "common.h"
#include <functional>

namespace R12 {

class DependencyViolation {
private:
	ServiceID m_s1;
	ServiceID m_s2;
	NeighborhoodID m_n;
public:
	DependencyViolation(ServiceID s1, ServiceID s2, NeighborhoodID n)
	: m_s1(s1), m_s2(s2), m_n(n) {
	}
	ServiceID s1() const { return m_s1; }
	ServiceID s2() const { return m_s2; }
	NeighborhoodID n() const { return m_n; }
	bool operator<(const DependencyViolation & v) const {
		if (&v == this) return false;
		if (s1() == v.s1()) {
			if (s2() == v.s2()) {
				return n() < v.n();
			}
			return s2() < v.s2();
		}
		return s1() < v.s1();
	}
	bool operator==(const DependencyViolation & v) const {
		if (&v == this) return true;
		return v.s1() == s1() && v.s2() == s2() && v.n() == n();
	}
};

struct DependencyViolationHash {
	std::size_t operator()(const DependencyViolation & v) const {
		std::hash<ServiceID> sHash;
		std::hash<NeighborhoodID> nHash;
		return sHash(v.s1()) ^ sHash(v.s2()) ^ nHash(v.n());
	}
};

}

#endif
