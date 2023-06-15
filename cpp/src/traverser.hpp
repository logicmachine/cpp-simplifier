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
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/ExprCXX.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <unordered_set>

#pragma once

class Traverser {
  public:
    Traverser(const clang::SourceManager &sm, const std::unordered_set<const clang::Decl *> &roots)
        : sm(sm), roots(roots) {}

    std::unordered_set<const clang::Decl *> get_traversed_decls() {
        for (const auto *const decl : roots)
            traverse(decl, 0);
        return decls;
    }

  private:
    const clang::SourceManager &sm;
    const std::unordered_set<const clang::Decl *> &roots;

    void debug(int depth, char type, const clang::Decl *decl) { debugDecl(depth, type, decl, sm); }

    std::unordered_set<const clang::Decl *> decls;
    std::unordered_set<const clang::Stmt *> stmts;
    std::unordered_set<const clang::Type *> types;

    void traverse(const clang::Decl *decl, int depth) {
        if (!decl || !decls.insert(decl).second) {
            return;
        }

        CTC_DEBUG(debug(depth, 'D', decl));

        // the declaration might be in a context
        if (const auto *const ctx = decl->getDeclContext()) {
            for (const auto &udir : ctx->using_directives()) {
                traverse(udir, depth + 1);
            }
            if (const auto *const ctx_decl = clang::dyn_cast<clang::Decl>(ctx)) {
                traverse(ctx_decl, depth + 1);
            }
        }

        // You only need to recurse on the special cases
        // Ordering here is important & brittle: subclasses need to be tested first

        test_and_traverse<clang::TypedefNameDecl>(decl, depth);
        test_and_traverse<clang::ClassTemplateSpecializationDecl>(decl, depth);
        test_and_traverse<clang::CXXRecordDecl>(decl, depth);
        test_and_traverse<clang::ClassTemplateDecl>(decl, depth);
        test_and_traverse<clang::TemplateDecl>(decl, depth);

        test_and_traverse<clang::ParmVarDecl>(decl, depth);
        test_and_traverse<clang::VarDecl>(decl, depth);
        test_and_traverse<clang::FieldDecl>(decl, depth);
        test_and_traverse<clang::EnumConstantDecl>(decl, depth);
        test_and_traverse<clang::CXXConstructorDecl>(decl, depth);
        test_and_traverse<clang::FunctionDecl>(decl, depth);
        test_and_traverse<clang::ValueDecl>(decl, depth);
    }
    template <typename T, typename U> void test_and_traverse(U *node, int depth) {
        if (clang::isa<T>(node)) {
            down_casted(clang::dyn_cast<T>(node), depth + 1);
        }
    }

    //------------------------------------------------------------------------
    // Declarations
    //------------------------------------------------------------------------
    void down_casted(const clang::TypedefNameDecl *decl, int depth) {
        // aliased type
        traverse(decl->getUnderlyingType(), depth);
    }
    void down_casted(const clang::CXXRecordDecl *decl, int depth) {
        if (!decl->hasDefinition()) {
            traverse(decl->getDefinition(), depth);
            return;
        }
        for (auto *const child : decl->decls()) {
            // access specifier
            if (clang::isa<clang::AccessSpecDecl>(child)) {
                traverse(child, depth);
            }
        }
        // base class
        for (const auto &base : decl->bases()) {
            traverse(base.getType(), depth);
        }
        // virtual base class
        for (const auto &vbase : decl->vbases()) {
            traverse(vbase.getType(), depth);
        }
        // user-defined destructor
        if (decl->hasUserDeclaredDestructor()) {
            traverse(decl->getDestructor(), depth);
        }
    }
    void down_casted(const clang::ClassTemplateSpecializationDecl *decl, int depth) {
        // specialization source class template
        traverse(decl->getSpecializedTemplate(), depth);
        // if the specialization source is a partially specialized class template
        const auto from = decl->getInstantiatedFrom();
        if (from.is<clang::ClassTemplatePartialSpecializationDecl *>()) {
            traverse(from.get<clang::ClassTemplatePartialSpecializationDecl *>(), depth);
        }
    }
    void down_casted(const clang::TemplateDecl *decl, int depth) {
        // template argument
        for (auto *const param : *(decl->getTemplateParameters())) {
            traverse(param, depth);
        }
    }
    void down_casted(const clang::ClassTemplateDecl *decl, int depth) {
        // specialization source record type
        traverse(decl->getTemplatedDecl(), depth);
    }

    void down_casted(const clang::ValueDecl *decl, int depth) {
        // corresponding type
        traverse(decl->getType(), depth);
    }
    void down_casted(const clang::FieldDecl *decl, int depth) {
        // width of bitfield
        if (decl->isBitField()) {
            traverse(decl->getBitWidth(), depth);
        }
        // member initializer
        if (decl->hasInClassInitializer()) {
            traverse(decl->getInClassInitializer(), depth);
        }
        // fixed-size array members can refer to other expressions for their length
        if (decl->getTypeSourceInfo()) {
            constantArrayHack(*decl->getTypeSourceInfo(), depth);
        }
    }
    void down_casted(const clang::EnumConstantDecl *decl, int depth) {
        traverse(decl->getType(), depth);
    }

    void down_casted(const clang::FunctionDecl *decl, int depth) {
        // template function specialization source
        if (decl->isFunctionTemplateSpecialization()) {
            traverse(decl->getPrimaryTemplate(), depth);
        }
        // argument
        for (unsigned int i = 0; i < decl->getNumParams(); ++i) {
            traverse(decl->getParamDecl(i), depth);
        }
        // processing content (body)
        if (decl->hasBody()) {
            traverse(decl->getBody(), depth);
        }
        // return type
        traverse(decl->getReturnType(), depth);
    }
    void down_casted(const clang::CXXConstructorDecl *decl, int depth) {
        // initialization list
        for (auto *const init : decl->inits()) {
            if (init->getMember()) {
                traverse(init->getMember(), depth);
            }
            traverse(init->getInit(), depth);
        }
    }
    void down_casted(const clang::VarDecl *decl, int depth) {
        // initialization expression
        if (decl->hasInit()) {
            traverse(decl->getInit(), depth);
        }
        // type intelligence
        // fixed-size array members can refer to other expressions for their length
        if (decl->getTypeSourceInfo()) {
            constantArrayHack(*decl->getTypeSourceInfo(), depth);
        }
    }
    void constantArrayHack(const clang::TypeSourceInfo &typeSourceInfo, int depth) {
        const auto &type = typeSourceInfo.getType();
        if (clang::isa<clang::ConstantArrayType>(type.getTypePtr())) {
            traverse(typeSourceInfo.getTypeLoc(), depth);
        } else {
            traverse(type, depth);
        }
    }
    void down_casted(const clang::ParmVarDecl *decl, int depth) {
        // default argument
        if (decl->hasDefaultArg()) {
            traverse(decl->getDefaultArg(), depth);
        }
        if (decl->getTypeSourceInfo()) {
            constantArrayHack(*decl->getTypeSourceInfo(), depth);
        }
    }

    //------------------------------------------------------------------------
    // Statements
    //------------------------------------------------------------------------
    void traverse(const clang::Stmt *stmt, int depth) {
        if (!stmt) {
            return;
        }
        if (!stmts.insert(stmt).second) {
            return;
        }

        CTC_DEBUG(debugStr(depth, "S",
                           std::string(stmt->getStmtClassName()) + " " +
                               RangeToString(stmt->getSourceRange(), sm)));

        for (const auto *const child : stmt->children()) {
            traverse(child, depth + 1);
        }
        test_and_traverse<clang::DeclStmt>(stmt, depth);
        test_and_traverse<clang::DeclRefExpr>(stmt, depth);
        test_and_traverse<clang::MemberExpr>(stmt, depth);
        test_and_traverse<clang::InitListExpr>(stmt, depth);
        test_and_traverse<clang::DesignatedInitExpr>(stmt, depth);
        test_and_traverse<clang::CallExpr>(stmt, depth);
        test_and_traverse<clang::CXXConstructExpr>(stmt, depth);
        test_and_traverse<clang::ExplicitCastExpr>(stmt, depth);
        test_and_traverse<clang::UnaryExprOrTypeTraitExpr>(stmt, depth);
    }

    void down_casted(const clang::DeclStmt *stmt, int depth) {
        // all definitions included as children
        for (auto *const decl : stmt->decls()) {
            traverse(decl, depth);
        }
    }
    void down_casted(const clang::DeclRefExpr *expr, int depth) {
        // the definition the reference points to
        traverse(expr->getDecl(), depth);
        // template argument
        for (unsigned int i = 0; i < expr->getNumTemplateArgs(); ++i) {
            traverse(expr->getTemplateArgs()[i], depth);
        }
        if (expr->hasQualifier()) {
            traverse(expr->getQualifier(), depth);
        }
    }
    void down_casted(const clang::MemberExpr *expr, int depth) {
        // member defintion
        traverse(expr->getMemberDecl(), depth);
        // template argument
        for (unsigned int i = 0; i < expr->getNumTemplateArgs(); ++i) {
            traverse(expr->getTemplateArgs()[i], depth);
        }
    }
    void down_casted(const clang::InitListExpr *expr, int depth) {
        if (auto *const syntactic = expr->getSyntacticForm()) {
            traverse(syntactic, depth);
        }
    }
    void down_casted(const clang::DesignatedInitExpr *expr, int depth) {
        for (const auto child : expr->designators()) {
            if (child.isFieldDesignator()) {
                traverse(child.getField(), depth + 1);
            }
        }
    }
    void down_casted(const clang::CallExpr *expr, int depth) {
        // function to be called
        traverse(expr->getCallee(), depth);
    }
    void down_casted(const clang::CXXConstructExpr *expr, int depth) {
        // constructor definition
        traverse(expr->getConstructor(), depth);
    }
    void down_casted(const clang::ExplicitCastExpr *expr, int depth) {
        // type to cast to
        traverse(expr->getTypeAsWritten(), depth);
    }
    void down_casted(const clang::UnaryExprOrTypeTraitExpr *expr, int depth) {
        if (expr->isArgumentType()) {
            traverse(expr->getArgumentType(), depth);
        } else {
            traverse(expr->getArgumentExpr(), depth);
        }
    }

    //------------------------------------------------------------------------
    // TypeLocs
    //------------------------------------------------------------------------
    template <typename T> void test_and_traverse(const clang::TypeLoc typeloc, int depth) {
        const auto typeloc2 = typeloc.getAsAdjusted<T>();
        if (!typeloc2.isNull())
            down_casted(typeloc2, depth);
    }

    void traverse(const clang::TypeLoc typeloc, int depth) {
        CTC_DEBUG(debugStr(depth, "TL", typeloc.getType()->getTypeClassName()));
        test_and_traverse<clang::ConstantArrayTypeLoc>(typeloc, depth + 1);
    }

    void down_casted(const clang::ConstantArrayTypeLoc typeloc, int depth) {
        traverse(typeloc.getInnerType(), depth);
        traverse(typeloc.getSizeExpr(), depth);
    }

    //------------------------------------------------------------------------
    // Types
    //------------------------------------------------------------------------
    void traverse(const clang::QualType &type, int depth) { traverse(type.getTypePtr(), depth); }
    void traverse(const clang::Type *type, int depth) {
        if (!type) {
            return;
        }
        if (!types.insert(type).second) {
            return;
        }

        CTC_DEBUG(debugStr(depth, "T", type->getTypeClassName()));

        test_and_traverse<clang::PointerType>(type, depth);
        test_and_traverse<clang::ReferenceType>(type, depth);
        test_and_traverse<clang::ConstantArrayType>(type, depth);
        test_and_traverse<clang::ArrayType>(type, depth);
        test_and_traverse<clang::AttributedType>(type, depth);
        test_and_traverse<clang::TypeOfType>(type, depth);
        test_and_traverse<clang::FunctionProtoType>(type, depth);
        test_and_traverse<clang::ParenType>(type, depth);
        test_and_traverse<clang::AutoType>(type, depth);
        test_and_traverse<clang::DecltypeType>(type, depth);
        test_and_traverse<clang::RecordType>(type, depth);
        test_and_traverse<clang::EnumType>(type, depth);
        test_and_traverse<clang::TypedefType>(type, depth);
        test_and_traverse<clang::TemplateSpecializationType>(type, depth);
        test_and_traverse<clang::ElaboratedType>(type, depth);
    }

    void down_casted(const clang::PointerType *type, int depth) {
        // dereferenced type
        traverse(type->getPointeeType(), depth);
    }
    void down_casted(const clang::ReferenceType *type, int depth) {
        // dereferenced type
        traverse(type->getPointeeType(), depth);
    }
    void down_casted(const clang::ArrayType *type, int depth) {
        // element type
        traverse(type->getElementType(), depth);
    }
    void down_casted(const clang::ConstantArrayType *type, int depth) {
        // element type
        traverse(type->getElementType(), depth);
    }
    void down_casted(const clang::AttributedType *type, int depth) {
        // qualified type
        traverse(type->getModifiedType(), depth);
    }
    void down_casted(const clang::TypeOfType *type, int depth) {
        traverse(type->getUnderlyingType(), depth);
    }
    void down_casted(const clang::FunctionProtoType *type, int depth) {
        for (const auto paramType : type->param_types()) {
            traverse(paramType, depth);
        }
        traverse(type->getReturnType(), depth);
    }
    void down_casted(const clang::ParenType *type, int depth) {
        traverse(type->getInnerType(), depth);
    }
    void down_casted(const clang::AutoType *type, int depth) {
        // result of type inference
        traverse(type->getDeducedType(), depth);
    }
    void down_casted(const clang::DecltypeType *type, int depth) {
        // formula used for inference
        traverse(type->getUnderlyingExpr(), depth);
        // result of type inference
        traverse(type->getUnderlyingType(), depth);
    }
    void down_casted(const clang::RecordType *type, int depth) {
        // structure definition
        traverse(type->getDecl(), depth);
    }
    void down_casted(const clang::EnumType *type, int depth) {
        // structure (enum?) definition
        traverse(type->getDecl(), depth);
    }
    void down_casted(const clang::TypedefType *type, int depth) {
        // alias definition
        traverse(type->getDecl(), depth);
    }
    void down_casted(const clang::TemplateSpecializationType *type, int depth) {
        const auto t = type->getTemplateName();
        traverse(t.getAsTemplateDecl(), depth);
        // template argument
        for (unsigned int i = 0; i < type->getNumArgs(); ++i) {
            traverse(type->getArgs()[i], depth);
        }
    }
    void down_casted(const clang::ElaboratedType *type, int depth) {
        traverse(type->getNamedType(), depth);
        if (type->getQualifier()) {
            traverse(type->getQualifier(), depth);
        }
    }

    //------------------------------------------------------------------------
    // Others
    //------------------------------------------------------------------------
    void traverse(const clang::NestedNameSpecifier *spec, int depth) {
        switch (spec->getKind()) {
        case clang::NestedNameSpecifier::TypeSpec:
        case clang::NestedNameSpecifier::TypeSpecWithTemplate:
            traverse(spec->getAsType(), depth);
            break;
        }
        if (spec->getPrefix()) {
            traverse(spec->getPrefix(), depth);
        }
    }

    void traverse(const clang::TemplateArgumentLoc &arg_loc, int depth) {
        const auto &arg = arg_loc.getArgument();
        ++depth;
        switch (arg.getKind()) {
        case clang::TemplateArgument::Type:
            traverse(arg_loc.getTypeSourceInfo()->getType(), depth);
            break;
        case clang::TemplateArgument::Expression:
            traverse(arg_loc.getSourceExpression(), depth);
            break;
        case clang::TemplateArgument::Declaration:
            traverse(arg_loc.getSourceDeclExpression(), depth);
            break;
        case clang::TemplateArgument::NullPtr:
            traverse(arg_loc.getSourceNullPtrExpression(), depth);
            break;
        case clang::TemplateArgument::Integral:
            traverse(arg_loc.getSourceIntegralExpression(), depth);
            break;
        }
    }

    void traverse(const clang::TemplateArgument &arg, int depth) {
        ++depth;
        switch (arg.getKind()) {
        case clang::TemplateArgument::Type:
            traverse(arg.getAsType(), depth);
            break;
        case clang::TemplateArgument::Expression:
            traverse(arg.getAsExpr(), depth);
            break;
        case clang::TemplateArgument::Declaration:
            traverse(arg.getAsDecl(), depth);
            break;
        case clang::TemplateArgument::NullPtr:
            traverse(arg.getNullPtrType(), depth);
            break;
        }
    }
};
