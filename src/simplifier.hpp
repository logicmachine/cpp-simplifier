#ifndef CPP_SIMPLIFIER_SIMPLIFIER_HPP
#define CPP_SIMPLIFIER_SIMPLIFIER_HPP

#include <string>
#include <unordered_set>

std::string simplify(
	clang::tooling::ClangTool &tool,
	const std::string &input_filname,
	const std::unordered_set<std::string> &roots);

#endif

