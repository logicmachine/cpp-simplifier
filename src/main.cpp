#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include "inclusion_unroller.hpp"
#include "simplifier.hpp"

std::string read_from_stream(std::istream &is){
	std::ostringstream oss;
	std::string line;
	while(std::getline(is, line)){ oss << line << std::endl; }
	return oss.str();
}

namespace cl = llvm::cl;
namespace tl = clang::tooling;

cl::OptionCategory simplifier_category("cpp-simplifier options");
cl::extrahelp common_help(tl::CommonOptionsParser::HelpMessage);
cl::opt<std::string> output_filename("o",
	cl::desc("Specify output filename"),
	cl::value_desc("filename"),
	cl::cat(simplifier_category));
cl::list<std::string> roots("r",
	cl::desc("Specify root function"),
	cl::value_desc("func"),
	cl::cat(simplifier_category));
bool debugOn;
cl::opt<bool, true> debugOpt("d",
	cl::desc("Enable debug output on stderr"),
	cl::location(debugOn),
	cl::cat(simplifier_category));

static const std::regex clang13_only_flags(
    "-Wno-psabi"
    "|-Wno-frame-address"
    "|-Wno-unused-but-set-variable"
    "|-Wno-pointer-to-enum-cast"
    "|-mstack-protector-guard=sysreg"
    "|-mstack-protector-guard-reg=sp_el0"
    "|-mstack-protector-guard-offset=1272"
);

static tl::CommandLineArguments remove_clang13_only_flags(
		const tl::CommandLineArguments& args,
		llvm::StringRef filename){
	tl::CommandLineArguments result(args.size());
	std::copy_if(args.begin(), args.end(), result.begin(), [](const std::string& s){
	        return !std::regex_match(s, clang13_only_flags);
	});
	result.push_back("-fparse-all-comments");
	return result;
}

int main(int argc, const char *argv[]){
	tl::CommonOptionsParser options_parser(argc, argv, simplifier_category);

	const auto& filenames = options_parser.getSourcePathList();
	if (filenames.size() > 1) {
		// TODO aggregate processing - merge the results of
		// processing each TranslationUnit
		std::cerr << "Warning: All files except " << filenames[0]
			<< " are being ignored." << std::endl;
	}

	std::string filename = filenames[0];
	std::string source;

	// Mostly to keep test infrastructure working
	if(filename == "-"){
		filename = "(stdin).cpp";
		source = read_from_stream(std::cin);
	}else{
		std::ifstream ifs(filename);
		source = read_from_stream(ifs);
	}

	tl::ClangTool tool(options_parser.getCompilations(), llvm::ArrayRef<std::string>(filename));
	tool.mapVirtualFile(filename, source);
	tool.appendArgumentsAdjuster(remove_clang13_only_flags);

	// TODO delete this (not needed, messing up locations)
	// const auto unrolled = unroll_inclusion(
	// 	input_source, input_filename, clang_options);

	const std::unordered_set<std::string> rootSet(roots.begin(), roots.end());
	const auto result = simplify(tool, filename, rootSet);

	if(!output_filename.empty()){
		std::ofstream ofs(output_filename.c_str());
		ofs << result;
	}else{
		std::cout << result;
	}
	return 0;
}

