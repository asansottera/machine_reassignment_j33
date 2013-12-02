#ifndef R12_PARAMETER_MAP_H
#define R12_PARAMETER_MAP_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>

namespace R12 {

class ParameterMap {
private:
	std::map<std::string,std::string> m_params;
public:
	ParameterMap() {
	}
	ParameterMap(const std::string config) {
		init(config);
	}
	void init(const std::string & config);
	bool hasParam(const std::string & key) const {
		return m_params.find(key) != m_params.end();
	}
	template<typename T> T param(const std::string & key) const {
		auto itr = m_params.find(key);
		if (itr == m_params.end()) {
			return T();
		} else {
			return boost::lexical_cast<T>(itr->second);
		}
	}
	template<typename T> T param(const std::string & key, const T def) const {
		auto itr = m_params.find(key);
		if (itr == m_params.end()) {
			return def;
		} else {
			return boost::lexical_cast<T>(itr->second);
		}
	}
	ParameterMap extractGroup(const std::string & prefix) const;
};

}

#endif
