#ifndef R12_EXCHANGE_H
#define R12_EXCHANGE_H

#include "common.h"
#include <iostream>

/*! Represents the movement of process 1 from machine 1 to machine 2
	and the movement of process 2 from machine 2 to machine 1. */
class Exchange {
private:
	MachineID m_m1;
	ProcessID m_p1;
	MachineID m_m2;
	ProcessID m_p2;
public:
	Exchange(const MachineID m1, const ProcessID p1, const MachineID m2, const ProcessID p2)
		: m_m1(m1), m_p1(p1), m_m2(m2), m_p2(p2) {
	}
	MachineID m1() const {
		return m_m1;
	}
	ProcessID p1() const {
		return m_p1;
	}
	MachineID m2() const {
		return m_m2;
	}
	ProcessID p2() const {
		return m_p2;
	}
	Exchange reverse() const {
		return Exchange(m1(), p2(), m2(), p1());
	}
};

inline std::ostream & operator<<(std::ostream & out, const Exchange & ex) {
	out << "Exchange: process " << ex.p1();
	out << " @ " << ex.m1();
	out << " and process " << ex.p2();
	out << " @ " << ex.m2();
	return out;
}

#endif
