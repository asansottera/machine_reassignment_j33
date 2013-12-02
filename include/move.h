#ifndef R12_MOVE_H
#define R12_MOVE_H

#include "common.h"
#include <iostream>

namespace R12 {

/*! Represents the movement of process p from machine src to machine dst.*/
class Move {
protected:
	ProcessID m_p;
	MachineID m_src;
	MachineID m_dst;
public:
	Move(ProcessID p, MachineID src, MachineID dst)
	: m_p(p), m_src(src), m_dst(dst) {
	}
	ProcessID p() const { return m_p; }
	MachineID src() const { return m_src; }
	MachineID dst() const { return m_dst; }
	Move reverse() const {
		return Move(p(), dst(), src());
	}
};

inline std::ostream & operator<<(std::ostream & out, const Move & move) {
	out << "Move: process " << move.p();
	out << " from " << move.src();
	out << " to " << move.dst();
	return out;
}

}

#endif
