#ifndef CPP_SIMPLIFIER_INCLUSION_UNROLLER_HPP
#define CPP_SIMPLIFIER_INCLUSION_UNROLLER_HPP

#include <string>
#include <vector>

std::string unroll_inclusion(
	const std::string &input_source,
	const std::string &input_filename,
	const std::vector<std::string> &clang_options);

#endif

