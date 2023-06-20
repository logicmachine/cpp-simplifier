// clang-format off
/************************************************************************************/
/*  The following parts of c-tree-carver contain new code released under the        */
/*  BSD 2-Clause License:                                                           */
/*  * `bin`                                                                         */
/*  * `cpp/src/debug.hpp`                                                           */
/*  * `cpp/src/debug_printers.cpp`                                                  */
/*  * `cpp/src/debug_printers.hpp`                                                  */
/*  * `cpp/src/source_range_hash.hpp`                                               */
/*  * `lib`                                                                         */
/*  * `test`                                                                        */
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

#include "simplifier.hpp"
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

// clang-format on

namespace fs = std::filesystem;
namespace cl = llvm::cl;
namespace tl = clang::tooling;

struct UnsignedOptParser : public cl::parser<std::optional<unsigned>> {
    UnsignedOptParser(cl::Option &o) : cl::parser<std::optional<unsigned>>(o){};

    static bool parse(cl::Option &o, llvm::StringRef arg_name, llvm::StringRef &arg,
                      std::optional<unsigned> &val) {
        cl::parser<unsigned> tmp(o);
        unsigned out = 0;
        const auto try_parse = tmp.parse(o, arg_name, arg, out);
        if (!try_parse)
            val = out;
        return try_parse;
    }
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
cl::OptionCategory simplifier_category("clang-tree-carve options");

cl::opt<std::optional<unsigned>, false, UnsignedOptParser>
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    commandOpt("n", cl::desc("For multiple commands, choose one of them"),
               cl::init(std::optional<unsigned>()), cl::cat(simplifier_category));

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables, cppcoreguidelines-interfaces-global-init)
cl::extrahelp common_help(tl::CommonOptionsParser::HelpMessage);

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
cl::list<std::string> roots("r", cl::desc("Specify root function"), cl::value_desc("func"),
                            cl::cat(simplifier_category));

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool debugOn;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
cl::opt<bool, true> debugOpt("d", cl::desc("Enable debug output on stderr"), cl::location(debugOn),
                             cl::cat(simplifier_category));

static const std::regex clang13_only_flags("|-Wno-unused-but-set-variable"
                                           "|-mstack-protector-guard=sysreg"
                                           "|-mstack-protector-guard-reg=sp_el0"
                                           "|-mstack-protector-guard-offset=1272");

static tl::CommandLineArguments remove_clang13_only_flags(const tl::CommandLineArguments &args,
                                                          llvm::StringRef filename) {
    tl::CommandLineArguments result(args.size());
    std::copy_if(args.begin(), args.end(), result.begin(),
                 [](const std::string &s) { return !std::regex_match(s, clang13_only_flags); });
    result.push_back("-Xclang");
    result.push_back("-detailed-preprocessing-record");
    return result;
}

std::optional<tl::CompileCommand> check_and_get_command(const std::string &filename,
                                                        const tl::CompilationDatabase &compile_db) {

    const auto abs_path = fs::absolute(fs::path(filename));
    const auto commands = compile_db.getCompileCommands(abs_path.string());

    if (commands.empty()) {
        std::cerr << "error: file not found in compilation database" << std::endl;
        return {};
    }

    if (!commandOpt) {
        if (commands.size() > 1) {
            std::cerr << "error: more than one command in database" << std::endl;
            std::cerr << "specify which one you want using -n" << std::endl << std::endl;
            for (int i = 0; i < commands.size(); i++) {
                std::cerr << i << ':';
                for (const auto &arg : commands[i].CommandLine)
                    std::cerr << ' ' << arg;
                std::cerr << std::endl << std::endl;
            }
            return {};
        }
        /* commands.size() == 1 */
        commandOpt = 0;
    }

    /* commandOpt.has_value() */
    if (!(0 <= *commandOpt && *commandOpt < commands.size())) {
        std::cerr << "error: -n " << *commandOpt << " out of bounds [0-" << commands.size() - 1
                  << "]" << std::endl;
        return {};
    }

    const auto result = commands[*commandOpt];
    if (!fs::is_regular_file(fs::status(result.Filename))) {
        std::cerr << "error: no such file: '" << result.Filename << "'" << std::endl;
        return {};
    }

    if (fs::path(result.Filename).is_absolute()) {
        std::cerr << "error: filename:  '" << result.Filename << "' should be relative"
                  << std::endl;
        std::cerr << "       directory: '" << result.Directory << "'" << std::endl;
        std::cerr << "       command:   '";
        auto it = result.CommandLine.begin();
        std::cerr << *(it++);
        while (it != result.CommandLine.end())
            std::cerr << ' ' << *(it++);
        std::cerr << std::endl;
        std::cerr << "fix command or compile_commands.json" << std::endl;
        return {};
    }

    return result;
}

std::string fixupErrMsg(const std::string errMsg) {
    std::regex re("\\[CommonOptionsParser\\]: ");
    return std::regex_replace(errMsg, re, "");
}

// FixedCompilationDatabase does too much futzing
class SingletonDb : public tl::CompilationDatabase {
    tl::CompileCommand cmd;

  public:
    SingletonDb(tl::CompileCommand cmd) : cmd(cmd) {}
    std::vector<tl::CompileCommand> getCompileCommands(llvm::StringRef) const override {
        return {cmd};
    }
    std::vector<std::string> getAllFiles() const override { return {cmd.Filename}; }
};

int main(int argc, const char *argv[]) {
    cl::SetVersionPrinter(
        std::function([](llvm::raw_ostream &) { std::cout << "0.1" << std::endl; }));

    auto options_parser =
        tl::CommonOptionsParser::create(argc, argv, simplifier_category, cl::OneOrMore);
    if (auto E = options_parser.takeError()) {
        std::cerr << fixupErrMsg(toString(std::move(E)));
        return 1;
    }

    const auto &filenames = options_parser->getSourcePathList();
    if (filenames.size() > 1) {
        std::cerr << "warning: ignoring all files except " << filenames[0] << std::endl;
    }

    const auto command = check_and_get_command(filenames[0], options_parser->getCompilations());
    if (!command)
        return 1;

    const SingletonDb db(*command); // first arg is reference, hence constructed here
    tl::ClangTool tool(db, llvm::ArrayRef<std::string>(command->Filename));
    tool.appendArgumentsAdjuster(remove_clang13_only_flags);

    if (const auto result =
            simplify(tool, std::unordered_set<std::string>(roots.begin(), roots.end()))) {
        std::cout << result->string();
        return 0;
    }

    return 1;
}
