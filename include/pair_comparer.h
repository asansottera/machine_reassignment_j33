#ifndef R12_PAIR_COMPARER_H
#define R12_PAIR_COMPARER_H

#include <utility>

namespace R12 {

template<class T, class Value, bool Decreasing = false>
class PairComparer {
	typedef std::pair<T,Value> Pair;
public:
	bool operator()(const Pair & a, const Pair & b) const {
		if (Decreasing) {
			return a.second > b.second;
		} else {
			return a.second < b.second;
		}
	}
};


}

#endif
