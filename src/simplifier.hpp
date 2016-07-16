#ifndef CPP_SIMPLIFIER_SIMPLIFIER_HPP
#define CPP_SIMPLIFIER_SIMPLIFIER_HPP

#include <string>
#include <vector>

std::string simplify(
	const std::string &input_source,
	const std::string &input_filename,
	const std::vector<std::string> &clang_options);

#endif

