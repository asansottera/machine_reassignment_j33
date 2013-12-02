#ifndef R12_VECTOR_COMPARER_H
#define R12_VECTOR_COMPARER_H

#include <vector>

namespace R12 {

template<class Index, class Value, bool Decreasing = false>
class VectorComparer {
	std::vector<Value> & m_values;
public:
	VectorComparer(std::vector<Value> & values) : m_values(values) {
	}
	bool operator()(const Index & a, const Index & b) const {
		if (Decreasing) {
			return m_values[a] > m_values[b];
		} else {
			return m_values[a] < m_values[b];
		}
	}
};

}

#endif
