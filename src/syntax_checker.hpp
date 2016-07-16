#ifndef CPP_SIMPLIFIER_SYNTAX_CHECKER_HPP
#define CPP_SIMPLIFIER_SYNTAX_CHECKER_HPP

#include <string>
#include <vector>

bool check_syntax(
	const std::string &input_source,
	const std::string &input_filename,
	const std::vector<std::string> &clang_options);

#endif

