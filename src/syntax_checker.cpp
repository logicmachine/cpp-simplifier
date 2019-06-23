#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Frontend/FrontendActions.h>
#include "syntax_checker.hpp"

bool check_syntax(
	const std::string &input_source,
	const std::string &input_filename,
	const std::vector<std::string> &clang_options)
{
	namespace tooling = clang::tooling;
	tooling::FixedCompilationDatabase compilations(".", clang_options);
	tooling::ClangTool tool(
		compilations, llvm::ArrayRef<std::string>(input_filename));
	tool.mapVirtualFile(input_filename, input_source);

	tool.appendArgumentsAdjuster(tooling::getClangSyntaxOnlyAdjuster());
	const auto result = tool.run(
		tooling::newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
	return result == 0;
}

