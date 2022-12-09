#include <sstream>
#include <memory>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/ArgumentsAdjusters.h>
#include "simplifier.hpp"
#include "reachability_analyzer.hpp"

namespace tl = clang::tooling;

std::string simplify(
	tl::ClangTool &tool,
	const std::string &input_source,
	const std::unordered_set<std::string> &roots)
{
	auto marker = std::make_shared<ReachabilityMarker>();
	ReachabilityAnalyzerFactory analyzer_factory(marker, roots);
	if(tool.run(&analyzer_factory) != 0){
		throw std::runtime_error("compilation error");
	}

	std::istringstream iss(input_source);
	std::ostringstream oss;
	for(unsigned int i = 0; !iss.eof(); ++i){
		std::string line;
		if(std::getline(iss, line)){
			unsigned int j = 0;
			while(j < line.size() && isspace(line[j])){ ++j; }
			if(line[j] == '#'){ marker->mark(i); }
			if((*marker)(i)){ oss << line << std::endl; }
		}
	}

	return oss.str();
}

