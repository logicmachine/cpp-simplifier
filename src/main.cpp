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
        clang_options.push_back("-ferror-limit=1");
        clang_options.push_back("-Wp,-MMD,arch/arm64/kvm/hyp/nvhe/../.pgtable.nvhe.o.d");
        clang_options.push_back("-nostdinc");
        clang_options.push_back("-isystem/home/dhruv/linux-x86/clang-r433403b/lib64/clang/13.0.3/include");
        clang_options.push_back("-I./arch/arm64/include");
        clang_options.push_back("-I./arch/arm64/include/generated");
        clang_options.push_back("-I./include");
        clang_options.push_back("-I./arch/arm64/include/uapi");
        clang_options.push_back("-I./arch/arm64/include/generated/uapi");
        clang_options.push_back("-I./include/uapi");
        clang_options.push_back("-I./include/generated/uapi");
        //clang_options.push_back("-includeinclude/linux/kconfig.h");
        //clang_options.push_back("-includeinclude/linux/compiler_types.h");
        clang_options.push_back("-D__KERNEL__");
        clang_options.push_back("-mlittle-endian");
        clang_options.push_back("-DKASAN_SHADOW_SCALE_SHIFT=");
        clang_options.push_back("-Qunused-arguments");
        clang_options.push_back("-fmacro-prefix-map=./=");
        clang_options.push_back("-Wall");
        clang_options.push_back("-Wundef");
        clang_options.push_back("-Wno-unused-value"); // new
        clang_options.push_back("-Werror=strict-prototypes");
        clang_options.push_back("-Wno-trigraphs");
        clang_options.push_back("-fno-strict-aliasing");
        clang_options.push_back("-fno-common");
        clang_options.push_back("-fshort-wchar");
        clang_options.push_back("-fno-PIE");
        //clang_options.push_back("-Werror=implicit-function-declaration");
        clang_options.push_back("-Wno-implicit-function-declaration"); // new
        clang_options.push_back("-Wno-misleading-indentation"); // new
        clang_options.push_back("-Werror=implicit-int");
        clang_options.push_back("-Werror=return-type");
        clang_options.push_back("-Wno-format-security");
        clang_options.push_back("-std=gnu89");
        clang_options.push_back("--target=aarch64-linux-gnu");
        clang_options.push_back("-integrated-as");
        clang_options.push_back("-Werror=unknown-warning-option");
        clang_options.push_back("-mgeneral-regs-only");
        clang_options.push_back("-DCONFIG_CC_HAS_K_CONSTRAINT=1");
        //clang_options.push_back("-Wno-psabi"); // not supported
        clang_options.push_back("-fno-asynchronous-unwind-tables");
        clang_options.push_back("-fno-unwind-tables");
        clang_options.push_back("-mbranch-protection=pac-ret+leaf+bti");
        clang_options.push_back("-Wa,-march=armv8.5-a");
        clang_options.push_back("-DARM64_ASM_ARCH=\"armv8.5-a\"");
        clang_options.push_back("-DKASAN_SHADOW_SCALE_SHIFT=");
        clang_options.push_back("-fno-delete-null-pointer-checks");
        //clang_options.push_back("-Wno-frame-address"); // not suppported
        clang_options.push_back("-Wno-address-of-packed-member");
        clang_options.push_back("-O2");
        clang_options.push_back("-Wframe-larger-than=2048");
        clang_options.push_back("-fstack-protector-strong");
        clang_options.push_back("-Wno-format-invalid-specifier");
        clang_options.push_back("-Wno-gnu");
        clang_options.push_back("-mno-global-merge");
        //clang_options.push_back("-Wno-unused-but-set-variable"); // not supported
        clang_options.push_back("-Wno-unused-const-variable");
        clang_options.push_back("-fno-omit-frame-pointer");
        clang_options.push_back("-fno-optimize-sibling-calls");
        clang_options.push_back("-g");
        clang_options.push_back("-Wdeclaration-after-statement");
        clang_options.push_back("-Wvla");
        clang_options.push_back("-Wno-pointer-sign");
        clang_options.push_back("-Wno-array-bounds");
        clang_options.push_back("-fno-strict-overflow");
        clang_options.push_back("-fno-stack-check");
        clang_options.push_back("-Werror=date-time");
        clang_options.push_back("-Werror=incompatible-pointer-types");
        clang_options.push_back("-Wno-initializer-overrides");
        clang_options.push_back("-Wno-format");
        clang_options.push_back("-Wno-sign-compare");
        clang_options.push_back("-Wno-format-zero-length");
        //clang_options.push_back("-Wno-pointer-to-enum-cast"); // not supported
        clang_options.push_back("-Wno-tautological-constant-out-of-range-compare");
        //clang_options.push_back("-mstack-protector-guard=sysreg"); // not supported
        //clang_options.push_back("-mstack-protector-guard-reg=sp_el0"); // not supported
        //clang_options.push_back("-mstack-protector-guard-offset=1272"); // not supported
        clang_options.push_back("-I./arch/arm64/kvm/hyp/include");
        clang_options.push_back("-fno-stack-protector");
        clang_options.push_back("-DDISABLE_BRANCH_PROFILING");
        clang_options.push_back("-D__KVM_NVHE_HYPERVISOR__");
        clang_options.push_back("-D__DISABLE_EXPORTS");
        clang_options.push_back("-D__DISABLE_TRACE_MMIO__");
        clang_options.push_back("-DKBUILD_MODFILE=\"arch/arm64/kvm/hyp/nvhe/pgtable.nvhe\"");
        clang_options.push_back("-DKBUILD_BASENAME=\"pgtable.nvhe\"");
        clang_options.push_back("-DKBUILD_MODNAME=\"pgtable.nvhe\"");
        clang_options.push_back("-D__KBUILD_MODNAME=kmod_pgtable.nvhe");

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

        // FIXME messing up locations
	// const auto unrolled = unroll_inclusion(
	// 	input_source, input_filename, clang_options);

	const auto result = simplify(input_source, input_filename, clang_options);

	if(vm.count("output")){
		const auto output_filename = vm["output"].as<std::string>();
		std::ofstream ofs(output_filename.c_str());
		ofs << result;
	}else{
		std::cout << result;
	}
	return 0;
}

