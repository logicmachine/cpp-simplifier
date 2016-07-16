#ifndef CPP_SIMPLIFIER_REACHABILITY_MARKER_HPP
#define CPP_SIMPLIFIER_REACHABILITY_MARKER_HPP

#include <vector>

class ReachabilityMarker {

private:
	std::vector<bool> m_marker;

public:
	ReachabilityMarker()
		: m_marker()
	{ }

	void mark(unsigned int line){
		while(m_marker.size() <= line){
			m_marker.push_back(false);
		}
		m_marker[line] = true;
	}

	void unmark(unsigned int line){
		if((*this)(line)){
			m_marker[line] = false;
		}
	}

	bool operator()(unsigned int line) const {
		if(line >= m_marker.size()){ return false; }
		return m_marker[line];
	}

};

#endif

