#ifndef CPP_SIMPLIFIER_REACHABILITY_MARKER_HPP
#define CPP_SIMPLIFIER_REACHABILITY_MARKER_HPP

#include <string>
#include <unordered_map>
#include <vector>

class ReachabilityMarker {

private:
	std::unordered_map<std::string, std::vector<bool>> m_map;

public:
	ReachabilityMarker() = default;

	void mark(const std::string file, unsigned int line){
		auto& marker = m_map[file];
		while(marker.size() <= line){
			marker.push_back(false);
		}
		marker[line] = true;
	}

	void unmark(const std::string file, unsigned int line){
		if((*this)(file, line)){
			auto& marker = m_map.at(file);
			marker[line] = false;
		}
	}

	bool operator()(const std::string file, unsigned int line) const {
		if (m_map.find(file) == m_map.end()){ return false; }
		const auto& marker = m_map.at(file);
		if(line >= marker.size()){ return false; }
		return marker[line];
	}

	auto begin() const noexcept {
		return m_map.begin();
	}

	auto end() const noexcept {
		return m_map.end();
	}

};

#endif

