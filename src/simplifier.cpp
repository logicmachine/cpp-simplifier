#include <fstream>
#include <sstream>
#include <memory>
#include <filesystem>
#include <iostream>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/ArgumentsAdjusters.h>
#include "simplifier.hpp"
#include "reachability_analyzer.hpp"

namespace tl = clang::tooling;
namespace fs = std::filesystem;

std::ofstream create(const fs::path& filepath){
	const auto newpath = fs::temp_directory_path() / filepath;
	fs::create_directories(fs::path(newpath).remove_filename());
	return std::ofstream(newpath);
}

std::string simplify(
	tl::ClangTool &tool,
	const std::string &input_filename,
	const std::unordered_set<std::string> &roots)
{
	auto marker = std::make_shared<ReachabilityMarker>();
	ReachabilityAnalyzerFactory analyzer_factory(marker, roots);
	if(tool.run(&analyzer_factory) != 0){
		throw std::runtime_error("compilation error");
	}

	std::ostringstream oss;
	for (const auto& it : *marker){
		const auto& filename = it.first;
		const auto also_oss = input_filename.find(filename) != std::string::npos;
		std::cerr << "Outputting: " << filename << std::endl;
		const fs::path filepath(filename);
		auto ofs = create(filepath);
		const auto& marked = it.second;
		std::ifstream iss(filename);
		for(unsigned int i = 0; i < marked.size() && !iss.eof(); ++i){
			std::string line;
			if(std::getline(iss, line)){
				unsigned int j = 0;
				while(j < line.size() && isspace(line[j])){ ++j; }
				if(line[j] == '#' || marked[i]){
					ofs << line << std::endl;
					if (also_oss) oss << line << std::endl;
				}
			}
		}
	}

	return oss.str();
}

