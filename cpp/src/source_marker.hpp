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
#include "debug.hpp"
#include "debug_printers.hpp"
#include "kept_lines.hpp"
#include "source_range_hash.hpp"
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/Basic/SourceManager.h>
#include <string>
#include <unordered_set>

#pragma once

class SourceMarker {
  public:
    static void keep_range(int depth, const clang::SourceRange &range,
                           const clang::SourceManager &sm, KeptLines &kept_lines) {

        const auto begin = sm.getPresumedLoc(range.getBegin());
        const auto end = sm.getPresumedLoc(range.getEnd());
        if (begin.isInvalid() || end.isInvalid() || begin.getFileID() != end.getFileID()) {
            CTC_DEBUG(debugStr(depth, "Skip marking invalid loc", ""));
            return;
        }

        CTC_DEBUG(debugStr(depth, "Mark " + RangeToString(range, sm), ""));
        const auto filename = std::string(begin.getFilename());
        for (unsigned int i = begin.getLine() - 1; i < end.getLine(); ++i) {
            kept_lines.keep(filename, i);
        }

        // If the definition locations being marked are in a header file,
        // then we also need to mark the chain of includes leading to that
        // definition being availabled and used.
        for (auto includedBy = sm.getPresumedLoc(begin.getIncludeLoc()); includedBy.isValid();
             includedBy = sm.getPresumedLoc(includedBy.getIncludeLoc())) {

            const auto filename = std::string(includedBy.getFilename());
            if (filename == "<built-in>")
                break;
            CTC_DEBUG(debugStr(
                depth + 1, "Incl " + filename + ":" + std::to_string(includedBy.getLine()), ""));
            kept_lines.keep(filename, includedBy.getLine() - 1);
        }
    }

    SourceMarker(const clang::SourceManager &sm,
                 const std::unordered_set<const clang::Decl *> &traversed,
                 const std::vector<const clang::Decl *> &roots, SourceRangeSet &ranges)
        : sm(sm), traversed(traversed), roots(roots), ranges(ranges) {}

    KeptLines get_kept_lines() {
        for (const auto *const root : roots)
            recurse(root, 0);
        return kept_lines;
    }

  private:
    const std::unordered_set<const clang::Decl *> traversed;
    const std::vector<const clang::Decl *> roots;
    const clang::SourceManager &sm;
    KeptLines kept_lines;
    SourceRangeSet &ranges;

    static void keep_range(const clang::SourceRange &range, const clang::SourceManager &sm,
                           KeptLines &kept_lines) {
        keep_range(1, range, sm, kept_lines);
    }
    void debug(int depth, char type, const clang::Decl *decl) { debugDecl(depth, type, decl, sm); }

    clang::SourceLocation PreviousLine(const clang::SourceLocation &loc) {
        const auto col = sm.getPresumedColumnNumber(loc);
        // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
        return loc.getLocWithOffset(-col);
    }

    //------------------------------------------------------------------------
    // Marking
    //------------------------------------------------------------------------
    template <typename T> bool test_and_mark(const clang::Decl *decl, int depth) {
        if (clang::isa<T>(decl)) {
            return keep_decl(clang::dyn_cast<T>(decl), depth + 1);
        }
        return false;
    }

    void keep_range(const clang::SourceRange &range) {
        keep_range(range, sm, kept_lines);
        ranges.insert(range);
    }

    void keep_range(const clang::SourceLocation &begin, const clang::SourceLocation &end) {
        keep_range(clang::SourceRange(begin, end));
    }

    clang::SourceLocation EndOfHead(const clang::Decl *decl) {
        auto min_loc = decl->getEndLoc();
        if (const auto *const context = clang::dyn_cast<clang::DeclContext>(decl)) {
            for (auto *const child : context->decls()) {
                if (child->isImplicit()) {
                    continue;
                }
                const auto a = sm.getPresumedLineNumber(min_loc);
                const auto b = sm.getPresumedLineNumber(child->getBeginLoc());
                if (b < a) {
                    min_loc = child->getBeginLoc();
                }
            }
            const auto a = sm.getPresumedLineNumber(decl->getBeginLoc());
            const auto b = sm.getPresumedLineNumber(min_loc);
            if (b > a) {
                min_loc = PreviousLine(min_loc);
            }
        }
        return min_loc;
    }

    bool MarkRecursiveFiltered(const clang::Decl *decl, int depth) {
        if (traversed.find(decl) == traversed.end()) {
            return false;
        }
        return recurse(decl, depth);
    }

    bool keep_decl(const clang::AccessSpecDecl *decl, int) {
        keep_range(decl->getBeginLoc(), decl->getEndLoc());
        return true;
    }
    bool keep_decl(const clang::UsingDirectiveDecl *decl, int) {
        keep_range(decl->getBeginLoc(), decl->getEndLoc());
        return true;
    }
    bool keep_decl(const clang::NamespaceDecl *decl, int depth) {
        bool result = false;
        for (auto *const child : decl->decls()) {
            result |= MarkRecursiveFiltered(child, depth);
        }
        if (result) {
            keep_range(clang::SourceRange(decl->getBeginLoc(), EndOfHead(decl)));
            keep_range(decl->getRBraceLoc(), decl->getEndLoc());
        }
        return result;
    }

    bool keep_decl(const clang::TypedefNameDecl *decl, int) {
        keep_range(decl->getBeginLoc(), decl->getEndLoc());
        return true;
    }
    bool keep_decl(const clang::TypeAliasTemplateDecl *decl, int) {
        keep_range(decl->getBeginLoc(), decl->getEndLoc());
        return true;
    }
    bool keep_decl(const clang::RecordDecl *decl, int depth) {
        keep_range(clang::SourceRange(decl->getOuterLocStart(), EndOfHead(decl)));
        keep_range(decl->getBraceRange().getEnd(), decl->getEndLoc());
        for (auto *const child : decl->decls()) {
            MarkRecursiveFiltered(child, depth);
        }
        return true;
    }
    bool keep_decl(const clang::EnumDecl *decl, int depth) {
        keep_range(clang::SourceRange(decl->getOuterLocStart(), EndOfHead(decl)));
        keep_range(decl->getBraceRange().getEnd(), decl->getEndLoc());
        // keeping this unfiltered to preserve enum values
        for (auto *const child : decl->decls()) {
            recurse(child, depth);
        }
        return true;
    }
    bool keep_decl(const clang::ClassTemplateDecl *decl, int depth) {
        bool result = recurse(decl->getTemplatedDecl(), depth);
        for (auto *const spec : decl->specializations()) {
            result |= recurse(spec, depth);
        }
        if (result) {
            auto template_tail = decl->getTemplatedDecl()->getBeginLoc();
            const auto a = sm.getPresumedLineNumber(decl->getBeginLoc());
            const auto b = sm.getPresumedLineNumber(template_tail);
            if (b > a) {
                template_tail = PreviousLine(template_tail);
            }
            keep_range(clang::SourceRange(decl->getBeginLoc(), template_tail));
        }
        return result;
    }
    bool keep_decl(const clang::ClassTemplateSpecializationDecl *decl, int depth) {
        const auto template_head = decl->getSourceRange().getBegin();
        const auto template_tail = decl->getOuterLocStart();
        keep_range(clang::SourceRange(template_head, template_tail));
        return true;
    }

    bool keep_decl(const clang::FieldDecl *decl, int depth) {
        keep_range(decl->getBeginLoc(), decl->getEndLoc());
        return true;
    }
    bool keep_decl(const clang::EnumConstantDecl *decl, int depth) {
        keep_range(decl->getBeginLoc(), decl->getEndLoc());
        return true;
    }
    bool keep_decl(const clang::FunctionDecl *decl, int depth) {
        keep_range(decl->getBeginLoc(), decl->getEndLoc());
        return true;
    }
    bool keep_decl(const clang::FunctionTemplateDecl *decl, int depth) {
        bool result = recurse(decl->getTemplatedDecl(), depth);
        for (auto *const spec : decl->specializations()) {
            result |= recurse(spec, depth);
        }
        if (result) {
            auto template_tail = decl->getTemplatedDecl()->getBeginLoc();
            const auto a = sm.getPresumedLineNumber(decl->getBeginLoc());
            const auto b = sm.getPresumedLineNumber(template_tail);
            if (b > a) {
                template_tail = PreviousLine(template_tail);
            }
            keep_range(clang::SourceRange(decl->getBeginLoc(), template_tail));
        }
        return result;
    }
    bool keep_decl(const clang::VarDecl *decl, int depth) {
        keep_range(decl->getBeginLoc(), decl->getEndLoc());
        return true;
    }

    bool recurse(const clang::Decl *decl, int depth) {

        CTC_DEBUG(debug(depth, 'M', decl));

        if (decl->isImplicit()) {
            return false;
        }

        bool result = false;
        result |= test_and_mark<clang::AccessSpecDecl>(decl, depth);
        result |= test_and_mark<clang::UsingDirectiveDecl>(decl, depth);
        result |= test_and_mark<clang::NamespaceDecl>(decl, depth);

        result |= test_and_mark<clang::TypedefNameDecl>(decl, depth);
        result |= test_and_mark<clang::TypeAliasTemplateDecl>(decl, depth);
        result |= test_and_mark<clang::RecordDecl>(decl, depth);
        result |= test_and_mark<clang::EnumDecl>(decl, depth);
        result |= test_and_mark<clang::ClassTemplateDecl>(decl, depth);
        result |= test_and_mark<clang::ClassTemplateSpecializationDecl>(decl, depth);

        result |= test_and_mark<clang::FieldDecl>(decl, depth);
        result |= test_and_mark<clang::EnumConstantDecl>(decl, depth);
        result |= test_and_mark<clang::FunctionDecl>(decl, depth);
        result |= test_and_mark<clang::FunctionTemplateDecl>(decl, depth);
        result |= test_and_mark<clang::VarDecl>(decl, depth);
        return result;
    }
};
