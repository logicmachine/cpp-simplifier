#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include "syntax_checker.hpp"
#include "inclusion_unroller.hpp"
#include "simplifier.hpp"

std::string read_from_stream(std::istream &is){
	std::ostringstream oss;
	std::string line;
	while(std::getline(is, line)){ oss << line << std::endl; }
	return oss.str();
}

int main(int argc, const char *argv[]){
	namespace po = boost::program_options;

	po::options_description general_options("cpp-simplifier");
	general_options.add_options()
		("help,h", "Display available options")
		("output,o",
			po::value<std::string>(),
			"Destination to write simplified code")
		("std",
			po::value<std::string>()->default_value("c++11"),
			"Language standard to process for")
		("include-path,I",
			po::value<std::vector<std::string>>()->composing(),
			"Add directory to include search path")
		("define,D",
			po::value<std::vector<std::string>>()->composing(),
			"Add macro definition before parsing");
	po::options_description hidden_options("hidden options");
	hidden_options.add_options()
		("input-file", po::value<std::string>(), "Input file");
	po::options_description all_options;
	all_options.add(general_options).add(hidden_options);

	po::positional_options_description positional_desc;
	positional_desc.add("input-file", -1);

	po::variables_map vm;
	po::store(
		po::command_line_parser(argc, argv)
			.options(all_options)
			.positional(positional_desc)
			.run(),
		vm);
	po::notify(vm);

	if(vm.count("help") || vm.count("input-file") == 0){
		std::cout << general_options << std::endl;
		return 1;
	}

	std::vector<std::string> clang_options;
	if(vm.count("std")){
		clang_options.push_back("-std=" + vm["std"].as<std::string>());
	}
	if(vm.count("include-path")){
		for(const auto &s : vm["include-path"].as<std::vector<std::string>>()){
			clang_options.push_back("-I" + s);
		}
	}
	if(vm.count("define")){
		for(const auto &s : vm["define"].as<std::vector<std::string>>()){
			clang_options.push_back("-D" + s);
		}
	}

	auto input_filename = vm["input-file"].as<std::string>();
	std::string input_source;
	if(input_filename == "-"){
		input_filename = "(stdin).cpp";
		input_source = read_from_stream(std::cin);
	}else{
		std::ifstream ifs(input_filename.c_str());
		input_source = read_from_stream(ifs);
	}

	const auto validity =
		check_syntax(input_source, input_filename, clang_options);
	if(!validity){ return -1; }

	const auto unrolled = unroll_inclusion(
		input_source, input_filename, clang_options);

	const auto result = simplify(unrolled, input_filename, clang_options);

	if(vm.count("output")){
		const auto output_filename = vm["output"].as<std::string>();
		std::ofstream ofs(output_filename.c_str());
		ofs << result;
	}else{
		std::cout << result;
	}
	return 0;
}

