/************************************************************************************/
/*  The following parts of C-simplifier contain new code released under the         */
/*  BSD 2-Clause License:                                                           */
/*  * `src/debug.hpp`                                                               */
/*                                                                                  */
/*  Copyright (c) 2022 Dhruv Makwana                                                */
/*  All rights reserved.                                                            */
/*                                                                                  */
/*  This software was developed by the University of Cambridge Computer             */
/*  Laboratory as part of the Rigorous Engineering of Mainstream Systems            */
/*  (REMS) project. This project has been partly funded by an EPSRC                 */
/*  Doctoral Training studentship. This project has been partly funded by           */
/*  Google. This project has received funding from the European Research            */
/*  Council (ERC) under the European Union's Horizon 2020 research and              */
/*  innovation programme (grant agreement No 789108, Advanced Grant                 */
/*  ELVER).                                                                         */
/*                                                                                  */
/*  BSD 2-Clause License                                                            */
/*                                                                                  */
/*  Redistribution and use in source and binary forms, with or without              */
/*  modification, are permitted provided that the following conditions              */
/*  are met:                                                                        */
/*  1. Redistributions of source code must retain the above copyright               */
/*     notice, this list of conditions and the following disclaimer.                */
/*  2. Redistributions in binary form must reproduce the above copyright            */
/*     notice, this list of conditions and the following disclaimer in              */
/*     the documentation and/or other materials provided with the                   */
/*     distribution.                                                                */
/*                                                                                  */
/*  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''              */
/*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED               */
/*  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A                 */
/*  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR             */
/*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,                    */
/*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT                */
/*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF                */
/*  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND             */
/*  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,              */
/*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT              */
/*  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF              */
/*  SUCH DAMAGE.                                                                    */
/*                                                                                  */
/*  All other parts involve adapted code, with the new code subject to the          */
/*  above BSD 2-Clause licence and the original code subject to its MIT             */
/*  licence.                                                                        */
/*                                                                                  */
/*  The MIT License (MIT)                                                           */
/*                                                                                  */
/*  Copyright (c) 2016 Takaaki Hiragushi                                            */
/*                                                                                  */
/*  Permission is hereby granted, free of charge, to any person obtaining a copy    */
/*  of this software and associated documentation files (the "Software"), to deal   */
/*  in the Software without restriction, including without limitation the rights    */
/*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       */
/*  copies of the Software, and to permit persons to whom the Software is           */
/*  furnished to do so, subject to the following conditions:                        */
/*                                                                                  */
/*  The above copyright notice and this permission notice shall be included in all  */
/*  copies or substantial portions of the Software.                                 */
/*                                                                                  */
/*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      */
/*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        */
/*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     */
/*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          */
/*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   */
/*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   */
/*  SOFTWARE.                                                                       */
/************************************************************************************/

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include "inclusion_unroller.hpp"
#include "simplifier.hpp"

namespace cl = llvm::cl;
namespace tl = clang::tooling;

cl::OptionCategory simplifier_category("c-simplifier options");
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
	result.push_back("-Xclang");
	result.push_back("-detailed-preprocessing-record");
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
	tl::ClangTool tool(options_parser.getCompilations(), llvm::ArrayRef<std::string>(filename));
	tool.appendArgumentsAdjuster(remove_clang13_only_flags);

	// TODO delete this (not needed, messing up locations)
	// TODO OR track all pp directive/macro sources
	// unroll_inclusion(tool, filename);

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

