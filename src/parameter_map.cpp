#include "parameter_map.h"

using namespace R12;

void ParameterMap::init(const std::string & config) {
	m_params.clear();
	std::vector<std::string> paramPairs;
	boost::split(paramPairs, config, boost::is_any_of(":"));
	for (auto itr = paramPairs.begin(); itr != paramPairs.end(); ++itr) {
		if (itr->length() > 0) {
			std::vector<std::string> keyAndValue;
			boost::split(keyAndValue, *itr, boost::is_any_of("="));
			if (keyAndValue.size() != 2) {
				std::stringstream msg;
				msg << "Invalid key-value pair in configuration string: " << *itr;
				throw std::runtime_error(msg.str());
			}
			std::string & key = keyAndValue[0];
			std::string & value = keyAndValue[1];
			m_params.insert(std::pair<std::string,std::string>(key, value));
		}
	}
}

ParameterMap ParameterMap::extractGroup(const std::string & prefix) const {
	ParameterMap group;
	std::string prefixWithSep = prefix + "@";
	for (auto itr = m_params.begin(); itr != m_params.end(); ++itr) {
		if (boost::starts_with(itr->first, prefixWithSep)) {
			std::vector<std::string> pieces;
			boost::split(pieces, itr->first, boost::is_any_of("@"));
			if (pieces.size() == 1) {
				continue;
			} else if (pieces.size() == 2) {
				group.m_params[pieces[1]] = itr->second;
			} else {
				std::stringstream msg;
				msg << "Invalid use of group marker character '@' in parameter '" << itr->first << "'";
				throw std::runtime_error(msg.str());
			}
		}
	}
	return group;
}
