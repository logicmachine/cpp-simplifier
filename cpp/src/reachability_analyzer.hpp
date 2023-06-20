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

// clang-format on

#pragma once

#include "debug.hpp"
#include "debug_printers.hpp"
#include "kept_lines.hpp"
#include "source_range_hash.hpp"
#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_set>

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class PPRecordNested : public clang::PPCallbacks {
  public:
    using RangeToSet = std::unordered_map<clang::SourceRange, SourceRangeSet, SourceRangeHash>;
    using RangeToRange =
        std::unordered_map<clang::SourceRange, clang::SourceRange, SourceRangeHash>;

  private:
    RangeToSet &m_macro_deps;
    RangeToRange &m_range_hack;
    const clang::SourceManager &m_sm;

  public:
    static clang::SourceRange defn_range(const clang::MacroDefinition &MD) {
        auto *macro_info = MD.getMacroInfo();
        assert(macro_info);
        return clang::SourceRange(macro_info->getDefinitionLoc(),
                                  macro_info->getDefinitionEndLoc());
    }

    void MacroDefined(const clang::Token &Id, const clang::MacroDirective *MD) override {
        assert(MD);
        const auto *macro_info = MD->getMacroInfo();
        assert(macro_info);
        auto orig =
            clang::SourceRange(macro_info->getDefinitionLoc(), macro_info->getDefinitionEndLoc());
        // orig.getEnd() is the starting point of the last macro token. For some
        // reason (I think that because of how C handles string literals with
        // whitespace and new lines before it, the source location for the end of
        // macro (like the example below) ends up being 1:15 rather than 2:1, which
        // lead to the previous wrong output, rather than the intended one.
        // Inserting a leading space made was a simple work-around, but actually
        // recording the (inclusive) location of the end of the last token fixes the
        // problem.
        //
        // input:
        // ```
        // 1|#define MACRO \
		// 2|"hi"
        // ```
        //
        // intended output:
        // ```
        // 1|#define MACRO \
		// 2|"hi"
        //
        /// previous wrong output:
        // ```
        // 1|#define MACRO \
		// 2|//-"hi"
        // ```
        //
        // Futhermore, it seems like the PreprocessingRecord is filled up first,
        // before any user-defined callbacks are run. This means that adjusting the
        // source range for macros in HandleMacroDefine is too late.
        if (const auto num_tokens = macro_info->getNumTokens(); num_tokens > 0) {
            const auto &last_token =
                macro_info->getReplacementToken(macro_info->getNumTokens() - 1);
            m_range_hack[orig] = clang::SourceRange(
                orig.getBegin(),
                // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
                last_token.getLocation().getLocWithOffset(last_token.getLength() - 1));
        } else {
            m_range_hack[orig] = clang::SourceRange(
                // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
                orig.getBegin(), orig.getBegin().getLocWithOffset(Id.getLength() - 1));
        }
    }

    void MacroExpands(const clang::Token &Id, const clang::MacroDefinition &MD,
                      clang::SourceRange Range, const clang::MacroArgs *Args) override {
        static bool assigned = false;
        static clang::SourceRange outer_macro_defn_range;
        if (!Id.getLocation().isMacroID()) {
            outer_macro_defn_range = defn_range(MD);
            assigned = true;
            CTC_DEBUG(std::cerr << "TL Macro: " << Id.getIdentifierInfo()->getNameStart()
                                << std::endl);
        } else {
            assert(assigned);
            int count = 0;
            for (auto loc = Id.getLocation(); loc.isMacroID();
                 loc = m_sm.getImmediateMacroCallerLoc(loc), count++) {
            }
            CTC_DEBUG(std::cerr << std::string(count * 2, ' ')
                                << Id.getIdentifierInfo()->getNameStart() << std::endl);
            m_macro_deps[outer_macro_defn_range].emplace(defn_range(MD));
        }
    }
    PPRecordNested(RangeToSet &macro_deps, RangeToRange &range_hack, const clang::SourceManager &sm)
        : m_macro_deps(macro_deps), m_range_hack(range_hack), m_sm(sm) {
        // for macros defined via the command line
        auto invalid = clang::SourceRange();
        m_range_hack[invalid] = invalid;
    }
    virtual ~PPRecordNested() {}
};

class ReachabilityAnalyzer : public clang::ASTFrontendAction {

  private:
    std::optional<KeptLines> &kept_lines;
    SourceRangeSet m_ranges;
    const std::unordered_set<std::string> &roots;

  protected:
    void ExecuteAction() override;

    void findMacroDefns(clang::PreprocessingRecord &pr, const clang::SourceManager &sm,
                        clang::SourceRange range, const PPRecordNested::RangeToSet &macro_deps,
                        const PPRecordNested::RangeToRange &range_hack, SourceRangeSet &marked);

  public:
    class ASTConsumer;

    ReachabilityAnalyzer(std::optional<KeptLines> &kept_lines,
                         const std::unordered_set<std::string> &roots);

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &ci,
                                                          llvm::StringRef in_file) override;
};

class ReachabilityAnalyzerFactory : public clang::tooling::FrontendActionFactory {

    std::optional<KeptLines> &kept_lines;
    const std::unordered_set<std::string> &roots;

  public:
    ReachabilityAnalyzerFactory(std::optional<KeptLines> &kept_lines,
                                const std::unordered_set<std::string> &roots);

    std::unique_ptr<clang::FrontendAction> create() override;
};
