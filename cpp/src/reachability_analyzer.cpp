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

#include "reachability_analyzer.hpp"
#include "debug_printers.hpp"
#include "source_marker.hpp"
#include "traverser.hpp"
#include <clang/AST/Decl.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <unordered_set>

class InputValidator {

    clang::DiagnosticsEngine &diagnostics;
    const clang::SourceManager &sm;
    const unsigned not_after_id;

    bool IsBefore(const clang::Decl *prev, const clang::Decl *curr) {
        const auto lhs = sm.getPresumedLoc(prev->getEndLoc());
        const auto rhs = sm.getPresumedLoc(curr->getBeginLoc());

        // only care about declarations from a source file
        // that too the same source file
        if (lhs.isInvalid() || rhs.isInvalid() || lhs.getFileID() != rhs.getFileID())
            return true;

        // easy case
        if (lhs.getLine() < rhs.getLine())
            return true;

        // declaration inside a macro
        const auto prev_begin = sm.getPresumedLoc(prev->getBeginLoc());
        const auto prev_end = lhs;
        const auto curr_begin = rhs;
        const auto curr_end = sm.getPresumedLoc(curr->getEndLoc());
        const auto eq = [](const clang::PresumedLoc &a, const clang::PresumedLoc &b) {
            return a.getLine() == b.getLine() && a.getColumn() == b.getColumn();
        };
        if (prev->getBeginLoc().isMacroID() && curr->getBeginLoc().isMacroID() &&
            eq(prev_begin, prev_end) && eq(prev_end, curr_begin) && eq(curr_begin, curr_end))
            return true;

        // typedef on struct definition
        if (clang::isa<clang::RecordDecl>(prev) && clang::isa<clang::TypedefNameDecl>(curr) &&
            curr->getSourceRange().fullyContains(prev->getSourceRange())) {
            return true;
        }

        if (clang::isa<clang::EmptyDecl>(curr)) {
            return true;
        }

        if (const auto *const func = clang::dyn_cast<clang::FunctionDecl>(curr)) {
            if (0 != func->getBuiltinID()) {
                return true;
            }
        }

        // extern {struct tag, enum} var(args);
        if ((clang::isa<clang::RecordDecl>(prev) || clang::isa<clang::EnumDecl>(prev)) &&
            (clang::isa<clang::VarDecl>(curr) || clang::isa<clang::FunctionDecl>(curr)) &&
            curr->getSourceRange().fullyContains(prev->getSourceRange())) {
            return true;
        }

        // multiple vars in one-line declarator: `T a, b;`
        if (clang::isa<clang::VarDecl>(curr) && clang::isa<clang::VarDecl>(prev) &&
            prev_begin.getLine() == prev_end.getLine() &&
            prev_end.getLine() == curr_begin.getLine() &&
            curr_begin.getLine() == curr_end.getLine()) {
            return true;
        }

        return false;
    }

  public:
    InputValidator(clang::DiagnosticsEngine &diagnostics, const clang::SourceManager &sm,
                   const unsigned not_after_id)
        : diagnostics(diagnostics), sm(sm), not_after_id(not_after_id) {}

    bool ToplevelOnDiffLines(const clang::TranslationUnitDecl *tu) {
        auto it = tu->decls_begin();
        const auto end = tu->decls_end();
        const auto name_of = [](auto *decl) {
            if (const auto named_decl = clang::dyn_cast<clang::NamedDecl>(decl)) {
                return named_decl->getNameAsString();
            }
            return std::string();
        };

        auto result = true;

        // empty case
        if (it == end)
            return result;

        // non-empty case
        auto *prev = *it;
        while (++it != end) {
            auto *const curr = *it;
            if (!IsBefore(prev, curr)) {
                result = false;
                diagnostics.Report(curr->getBeginLoc(), not_after_id)
                    << name_of(prev) << name_of(curr);
            }
            prev = curr;
        }

        return result;
    }
};
class ReachabilityAnalyzer::ASTConsumer : public clang::ASTConsumer {

  private:
    const std::unordered_set<std::string> &roots;
    std::optional<KeptLines> &kept_lines;
    SourceRangeSet &m_ranges;

    static bool from_main_file(const clang::FunctionDecl *func_decl,
                               const clang::SourceManager &sm) {

        const auto begin = sm.getPresumedLoc(func_decl->getBeginLoc());
        const auto end = sm.getPresumedLoc(func_decl->getEndLoc());
        if (begin.isInvalid() || end.isInvalid() || begin.getFileID() != end.getFileID()) {
            return false;
        }

        if (auto includedBy = sm.getPresumedLoc(begin.getIncludeLoc()); includedBy.isValid()) {
            return false;
        }

        return true;
    }

  public:
    ASTConsumer(std::optional<KeptLines> &kept_lines, const std::unordered_set<std::string> &roots,
                SourceRangeSet &ranges)
        : kept_lines(kept_lines), roots(roots), m_ranges(ranges) {}

    std::unordered_set<const clang::Decl *> getFuncRoots(const clang::TranslationUnitDecl *tu,
                                                         const clang::IdentifierTable &idents,
                                                         const clang::SourceManager &sm) {

        std::unordered_set<const clang::Decl *> result;

        if (roots.empty()) {
            for (auto *const decl : tu->decls()) {
                if (auto *const func_decl = clang::dyn_cast<clang::FunctionDecl>(decl)) {
                    if (from_main_file(func_decl, sm))
                        result.insert(func_decl);
                }
            }
            return result;
        }

        for (const auto root : roots) {
            bool inserted = false;
            if (const auto maybeFound = idents.find(root); maybeFound != idents.end()) {
                const auto lookupResult =
                    tu->lookup(clang::DeclarationName(maybeFound->getValue()));
                for (auto *const named_decl : lookupResult) {
                    if (auto *const func_decl = named_decl->getAsFunction()) {
                        result.insert(func_decl);
                        inserted = true;
                    }
                }
            }
            if (!inserted) {
                std::cerr << "error: root '" << root << "' not found." << std::endl;
            }
        }
        return result;
    }

    void HandleTranslationUnit(clang::ASTContext &context) override {
        const auto &sm = context.getSourceManager();
        auto *const tu = context.getTranslationUnitDecl();
        auto &diagnostics = context.getDiagnostics();
        const auto not_after_id = diagnostics.getCustomDiagID(
            clang::DiagnosticsEngine::Error, "declaration '%1' must start on a line "
                                             "after the end of declaration '%0'");

        // CTC_DEBUG(tu->dump());

        if (!InputValidator{diagnostics, sm, not_after_id}.ToplevelOnDiffLines(tu))
            return;

        const auto roots = getFuncRoots(tu, context.Idents, sm);
        const auto transitive = Traverser{sm, roots}.get_traversed_decls();

        std::vector<const clang::Decl *> to_mark;
        std::copy_if(tu->decls_begin(), tu->decls_end(), std::back_inserter(to_mark),
                     [&transitive](const clang::Decl *decl) {
                         return (transitive.find(decl) != transitive.end() ||
                                 isAFwdDeclWhoseDefIsTraversed(decl, transitive));
                     });
        kept_lines = SourceMarker{sm, transitive, to_mark, m_ranges}.get_kept_lines();
    }

    // TODO This can't be the best/correct way...
    // TODO check if this handles function foward declarations
    static bool
    isAFwdDeclWhoseDefIsTraversed(const clang::Decl *decl,
                                  const std::unordered_set<const clang::Decl *> &traversed) {
        if (const auto *const tagDecl = clang::dyn_cast<clang::TagDecl>(decl)) {
            if (auto *const maybeDef = tagDecl->getDefinition()) {
                if (traversed.find(maybeDef) != traversed.end()) {
                    return true;
                }
            }
        }
        return false;
    }
};

ReachabilityAnalyzer::ReachabilityAnalyzer(std::optional<KeptLines> &kept_lines,
                                           const std::unordered_set<std::string> &roots)
    : kept_lines(kept_lines), roots(roots) {}

std::unique_ptr<clang::ASTConsumer>
ReachabilityAnalyzer::CreateASTConsumer(clang::CompilerInstance &ci, llvm::StringRef in_file) {
    return std::make_unique<ReachabilityAnalyzer::ASTConsumer>(kept_lines, roots, m_ranges);
}

bool actualFile(clang::SourceRange defn_range, const clang::SourceManager &sm) {
    const auto presumed = sm.getPresumedLoc(defn_range.getBegin());
    assert(presumed.isValid());
    auto result = std::strcmp(presumed.getFilename(), "<built-in>") != 0 &&
                  std::strcmp(presumed.getFilename(), "<command line>") != 0;
    if (!result)
        CTC_DEBUG(debugStr(2, "Not from a file", ""));
    return result;
}

void ReachabilityAnalyzer::findMacroDefns(clang::PreprocessingRecord &pr,
                                          const clang::SourceManager &sm, clang::SourceRange range,
                                          const PPRecordNested::RangeToSet &macro_deps,
                                          const PPRecordNested::RangeToRange &range_hack,
                                          SourceRangeSet &marked) {

    CTC_DEBUG(debugStr(0, "Macro expansions in " + RangeToString(range, sm), ""));
    const auto inRange = pr.getPreprocessedEntitiesInRange(range);
    for (const auto *entity : inRange) {
        assert(entity);
        if (const auto *const macro_exp = clang::dyn_cast<clang::MacroExpansion>(entity)) {
            CTC_DEBUG(debugStr(1, macro_exp->getName()->getNameStart(), ""));
            CTC_DEBUG(
                debugStr(2, "Expanded at " + RangeToString(macro_exp->getSourceRange(), sm), ""));
            const auto defn_range = macro_exp->getDefinition()->getSourceRange();
            CTC_DEBUG(debugStr(2, "Defined " + RangeToString(defn_range, sm), ""));

            // Not built-in, from command line, nor already marked
            if (actualFile(defn_range, sm) && marked.find(defn_range) == marked.end()) {
                SourceMarker::keep_range(2, range_hack.at(defn_range), sm, *kept_lines);
                marked.emplace(defn_range);
            }

            const auto dep_ranges = macro_deps.find(defn_range);
            // skip if macro has no dependencies
            if (dep_ranges == macro_deps.end())
                continue;

            // mark all macros used by this macro
            for (const auto &dep_range : dep_ranges->second) {
                CTC_DEBUG(debugStr(2, "Uses " + RangeToString(dep_range, sm), ""));
                if (marked.find(dep_range) == marked.end()) {
                    SourceMarker::keep_range(3, range_hack.at(dep_range), sm, *kept_lines);
                    marked.emplace(dep_range);
                }
            }
        }
    }
}

// 1. The PreprocessingRecord only inlcudes the Macro expanded when typed into
// the source,
//    not any subsequent macros expanded _inside a macro_.
// 2. None of
// SourceManager::{getFileLoc,getExpansionLoc,get(Immediate)SpellingLoc}()
// enable
//    to go from the source _down_/expand the Macros, only to go from a fully
//    expanded location back up to the source.
// 3. It seems difficult/not sensible to re-lex/pp the macro _definition_ to get
// at the macros
//    it refers to.
// Therefore, we add a PPCallback that records _all_ macro expansions, even
// those that occur within a macro.
void ReachabilityAnalyzer::ExecuteAction() {
    auto &ci = getCompilerInstance();
    auto &pp = ci.getPreprocessor();
    PPRecordNested::RangeToSet macro_deps;
    PPRecordNested::RangeToRange range_hack;
    const auto &sm = ci.getSourceManager();
    pp.addPPCallbacks(std::make_unique<PPRecordNested>(macro_deps, range_hack, sm));

    // this runs the pre-processor and declaration traversal & marking
    ASTFrontendAction::ExecuteAction();

    if (!kept_lines)
        CTC_DEBUG(std::cerr << "No preprocessing record found :(" << std::endl);

    // you need this for `PreprocessingRecord::getPreprocessedEntitiesInRange`
    auto *pr = pp.getPreprocessingRecord();
    if (!pr) {
        CTC_DEBUG(std::cerr << "No preprocessing record found :(" << std::endl);
        return;
    }

    // mark the marcros used inside all declaration ranges
    SourceRangeSet marked;
    for (const auto &range : m_ranges)
        findMacroDefns(*pr, sm, range, macro_deps, range_hack, marked);
}

ReachabilityAnalyzerFactory::ReachabilityAnalyzerFactory(
    std::optional<KeptLines> &kept_lines, const std::unordered_set<std::string> &roots)
    : kept_lines(kept_lines), roots(roots) {}

std::unique_ptr<clang::FrontendAction> ReachabilityAnalyzerFactory::create() {
    return std::make_unique<ReachabilityAnalyzer>(kept_lines, roots);
}
