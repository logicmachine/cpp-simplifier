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

#include <cstring>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/ExprCXX.h>
#include "reachability_analyzer.hpp"

// getPresumedLoc has an optional boolean UseLineDirectives
// should be useful if we ever want to handle preprocessed files
std::string RangeToString(clang::SourceRange range, const clang::SourceManager& sm){
	const auto begin = sm.getPresumedLoc(range.getBegin());
	const auto end = sm.getPresumedLoc(range.getEnd());
	if (!begin.isValid() || !end.isValid()) { return std::string("invalid"); }
	assert(begin.getFileID() == end.getFileID());
	std::ostringstream result;
	result << begin.getFilename() << ", " << begin.getLine() << ':' << begin.getColumn() << " - "
		<< end.getLine() << ':' << end.getColumn();
	return result.str();
}

void debugDecl(int depth, char type, const clang::Decl* decl, const clang::SourceManager& sm){
	std::cerr << std::string(depth * 2, ' ')
	          << type << ": " << decl->getDeclKindName();
	if(const auto named_decl = clang::dyn_cast<clang::NamedDecl>(decl)){
		const auto name = named_decl->getNameAsString();
		std::cerr << " (" << name << ") at ";
	}

	std::cerr << RangeToString(decl->getSourceRange(), sm) << std::endl;
}

void debugStr(int depth, const std::string type, const std::string& info){
	std::cerr << std::string(depth * 2, ' ') << type << ": " << info << std::endl;
}

void MarkRangeDSM(int depth, const clang::SourceRange& range, const clang::SourceManager& sm,
	const std::shared_ptr<ReachabilityMarker> marker){

	const auto begin = sm.getPresumedLoc(range.getBegin());
	const auto end = sm.getPresumedLoc(range.getEnd());
	if (begin.isInvalid() || end.isInvalid() || begin.getFileID() != end.getFileID()){
		SIMP_DEBUG(debugStr(depth, "Skip marking invalid loc", ""));
		return;
	}

	SIMP_DEBUG(debugStr(depth, "Mark " + RangeToString(range, sm), ""));
	const auto filename = std::string(begin.getFilename());
	for(unsigned int i = begin.getLine() - 1; i < end.getLine(); ++i){
		marker->mark(filename, i);
	}

	// If the definition locations being marked are in a header file,
	// then we also need to mark the chain of includes leading to that
	// definition being availabled and used.
	for (auto includedBy = sm.getPresumedLoc(begin.getIncludeLoc());
		includedBy.isValid();
		includedBy = sm.getPresumedLoc(includedBy.getIncludeLoc())) {

		const auto filename = std::string(includedBy.getFilename());
		if (filename == "<built-in>") break;
		SIMP_DEBUG(debugStr(depth + 1, "Incl " + filename + ":" + std::to_string(includedBy.getLine()), ""));
		marker->mark(filename, includedBy.getLine() - 1);
	}
}

void MarkRangeSM(const clang::SourceRange& range, const clang::SourceManager& sm,
	const std::shared_ptr<ReachabilityMarker> marker){
	MarkRangeDSM(1, range, sm, marker);
}

class ReachabilityAnalyzer::ASTConsumer : public clang::ASTConsumer {

private:
	const clang::SourceManager* m_source_manager;
	std::unordered_set<const clang::Decl *> m_traversed_decls;
	std::unordered_set<const clang::Stmt *> m_traversed_stmts;
	std::unordered_set<const clang::Type *> m_traversed_types;

	std::shared_ptr<ReachabilityMarker> m_marker;
	SourceRangeSet& m_ranges;
	const std::unordered_set<std::string>& m_roots;

	void reset(){
		m_traversed_decls.clear();
		m_traversed_stmts.clear();
		m_traversed_types.clear();
	}

	template <typename T, typename U>
	void TestAndTraverse(U *node, int depth){
		if(clang::isa<T>(node)){
			TraverseDetail(clang::dyn_cast<T>(node), depth + 1);
		}
	}

	clang::SourceLocation PreviousLine(const clang::SourceLocation &loc){
		const int col = m_source_manager->getPresumedColumnNumber(loc);
		return loc.getLocWithOffset(-col);
	}

	void debug(int depth, char type, const clang::Decl* decl){
		debugDecl(depth, type, decl, *m_source_manager);
	}
	//------------------------------------------------------------------------
	// Declarations
	//------------------------------------------------------------------------
	void Traverse(const clang::Decl *decl, int depth){
		if(!decl || !m_traversed_decls.insert(decl).second){ return; }

		SIMP_DEBUG(debug(depth, 'D', decl));

		// the declaration might be in a context
		if (const auto ctx = decl->getDeclContext()){
			for (const auto& udir : ctx->using_directives()) {
				Traverse(udir, depth+1);
			}
			if (const auto ctx_decl = clang::dyn_cast<clang::Decl>(ctx)){
				Traverse(ctx_decl, depth + 1);
			}
		}

		// You only need to recurse on the special cases
		// Ordering here is important & brittle: subclasses need to be tested first

		TestAndTraverse<clang::TypedefNameDecl>(decl, depth);
			TestAndTraverse<clang::ClassTemplateSpecializationDecl>(decl, depth);
		TestAndTraverse<clang::CXXRecordDecl>(decl, depth);
			TestAndTraverse<clang::ClassTemplateDecl>(decl, depth);
		TestAndTraverse<clang::TemplateDecl>(decl, depth);

				TestAndTraverse<clang::ParmVarDecl>(decl, depth);
				TestAndTraverse<clang::VarDecl>(decl, depth);
			TestAndTraverse<clang::FieldDecl>(decl, depth);
			TestAndTraverse<clang::EnumConstantDecl >(decl, depth);
				TestAndTraverse<clang::CXXConstructorDecl>(decl, depth);
			TestAndTraverse<clang::FunctionDecl>(decl, depth);
		TestAndTraverse<clang::ValueDecl>(decl, depth);
	}

	void TraverseDetail(const clang::TypedefNameDecl *decl, int depth){
		// aliased type
		Traverse(decl->getUnderlyingType(), depth);
	}
	void TraverseDetail(const clang::CXXRecordDecl *decl, int depth){
		if(!decl->hasDefinition()){
			Traverse(decl->getDefinition(), depth);
			return;
		}
		for(const auto child : decl->decls()){
			// access specifier
			if(clang::isa<clang::AccessSpecDecl>(child)){ Traverse(child, depth); }
		}
		// base class
		for(const auto &base : decl->bases()){ Traverse(base.getType(), depth); }
		// virtual base class
		for(const auto &vbase : decl->vbases()){ Traverse(vbase.getType(), depth); }
		// user-defined destructor
		if(decl->hasUserDeclaredDestructor()){
			Traverse(decl->getDestructor(), depth);
		}
	}
	void TraverseDetail(const clang::ClassTemplateSpecializationDecl *decl, int depth){
		// specialization source class template
		Traverse(decl->getSpecializedTemplate(), depth);
		// if the specialization source is a partially specialized class template
		const auto from = decl->getInstantiatedFrom();
		if(from.is<clang::ClassTemplatePartialSpecializationDecl *>()){
			Traverse(from.get<clang::ClassTemplatePartialSpecializationDecl *>(), depth);
		}
	}
	void TraverseDetail(const clang::TemplateDecl *decl, int depth){
		// template argument
		for(const auto param : *(decl->getTemplateParameters())){
			Traverse(param, depth);
		}
	}
	void TraverseDetail(const clang::ClassTemplateDecl *decl, int depth){
		// specialization source record type
		Traverse(decl->getTemplatedDecl(), depth);
	}

	void TraverseDetail(const clang::ValueDecl *decl, int depth){
		// corresponding type
		Traverse(decl->getType(), depth);
	}
	void TraverseDetail(const clang::FieldDecl *decl, int depth){
		// width of bitfield
		if(decl->isBitField()){ Traverse(decl->getBitWidth(), depth); }
		// member initializer
		if(decl->hasInClassInitializer()){
			Traverse(decl->getInClassInitializer(), depth);
		}
		// fixed-size array members can refer to other expressions for their length
		if(decl->getTypeSourceInfo()){
			constantArrayHack(*decl->getTypeSourceInfo(), depth);
		}
	}
	void TraverseDetail(const clang::EnumConstantDecl *decl, int depth){
		Traverse(decl->getType(), depth);
	}

	void TraverseDetail(const clang::FunctionDecl *decl, int depth){
		// template function specialization source
		if(decl->isFunctionTemplateSpecialization()){
			Traverse(decl->getPrimaryTemplate(), depth);
		}
		// argument
		for(unsigned int i = 0; i < decl->getNumParams(); ++i){
			Traverse(decl->getParamDecl(i), depth);
		}
		// processing content (body)
		if(decl->hasBody()){ Traverse(decl->getBody(), depth); }
		// return type
		Traverse(decl->getReturnType(), depth);
	}
	void TraverseDetail(const clang::CXXConstructorDecl *decl, int depth){
		// initialization list
		for(const auto init : decl->inits()){
			if(init->getMember()){ Traverse(init->getMember(), depth); }
			Traverse(init->getInit(), depth);
		}
	}
	void TraverseDetail(const clang::VarDecl *decl, int depth){
		// initialization expression
		if(decl->hasInit()){ Traverse(decl->getInit(), depth); }
		// type intelligence
		// fixed-size array members can refer to other expressions for their length
		if(decl->getTypeSourceInfo()){
			constantArrayHack(*decl->getTypeSourceInfo(), depth);
		}
	}
	void constantArrayHack(const clang::TypeSourceInfo& typeSourceInfo, int depth){
		const auto& type = typeSourceInfo.getType();
		if (clang::isa<clang::ConstantArrayType>(type.getTypePtr())){
			Traverse(typeSourceInfo.getTypeLoc(), depth);
		} else {
			Traverse(type, depth);
		}
	}
	void TraverseDetail(const clang::ParmVarDecl *decl, int depth){
		// default argument
		if(decl->hasDefaultArg()){ Traverse(decl->getDefaultArg(), depth); }
		if(decl->getTypeSourceInfo()){
			constantArrayHack(*decl->getTypeSourceInfo(), depth);
		}
	}

	//------------------------------------------------------------------------
	// Statements
	//------------------------------------------------------------------------
	void Traverse(const clang::Stmt *stmt, int depth){
		if(!stmt){ return; }
		if(!m_traversed_stmts.insert(stmt).second){ return; }

		SIMP_DEBUG(debugStr(depth, "S", std::string(stmt->getStmtClassName()) + " " + RangeToString(stmt->getSourceRange(), *m_source_manager)));

		for(const auto child : stmt->children()){
			Traverse(child, depth + 1);
		}
		TestAndTraverse<clang::DeclStmt>(stmt, depth);
		TestAndTraverse<clang::DeclRefExpr>(stmt, depth);
		TestAndTraverse<clang::MemberExpr>(stmt, depth);
		TestAndTraverse<clang::InitListExpr>(stmt, depth);
		TestAndTraverse<clang::DesignatedInitExpr>(stmt, depth);
		TestAndTraverse<clang::CallExpr>(stmt, depth);
		TestAndTraverse<clang::CXXConstructExpr>(stmt, depth);
		TestAndTraverse<clang::ExplicitCastExpr>(stmt, depth);
		TestAndTraverse<clang::UnaryExprOrTypeTraitExpr>(stmt, depth);
	}

	void TraverseDetail(const clang::DeclStmt *stmt, int depth){
		// all definitions included as children
		for(const auto decl : stmt->decls()){ Traverse(decl, depth); }
	}
	void TraverseDetail(const clang::DeclRefExpr *expr, int depth){
		// the definition the reference points to
		Traverse(expr->getDecl(), depth);
		// template argument
		for(unsigned int i = 0; i < expr->getNumTemplateArgs(); ++i){
			Traverse(expr->getTemplateArgs()[i], depth);
		}
		if(expr->hasQualifier()){
			Traverse(expr->getQualifier(), depth);
		}
	}
	void TraverseDetail(const clang::MemberExpr *expr, int depth){
		// member defintion
		Traverse(expr->getMemberDecl(), depth);
		// template argument
		for(unsigned int i = 0; i < expr->getNumTemplateArgs(); ++i){
			Traverse(expr->getTemplateArgs()[i], depth);
		}
	}
	void TraverseDetail(const clang::InitListExpr *expr, int depth){
		if(const auto syntactic = expr->getSyntacticForm()) {
			Traverse(syntactic, depth);
		}
	}
	void TraverseDetail(const clang::DesignatedInitExpr *expr, int depth){
		for(const auto child : expr->designators()){
			if(child.isFieldDesignator()){
				Traverse(child.getField(), depth + 1);
			}
		}
	}
	void TraverseDetail(const clang::CallExpr *expr, int depth){
		// function to be called
		Traverse(expr->getCallee(), depth);
	}
	void TraverseDetail(const clang::CXXConstructExpr *expr, int depth){
		// constructor definition
		Traverse(expr->getConstructor(), depth);
	}
	void TraverseDetail(const clang::ExplicitCastExpr *expr, int depth){
		// type to cast to
		Traverse(expr->getTypeAsWritten(), depth);
	}
	void TraverseDetail(const clang::UnaryExprOrTypeTraitExpr *expr, int depth){
		if(expr->isArgumentType()){
			Traverse(expr->getArgumentType(), depth);
		}else{
			Traverse(expr->getArgumentExpr(), depth);
		}
	}

	//------------------------------------------------------------------------
	// TypeLocs
	//------------------------------------------------------------------------
	template<typename T>
	void TestAndTraverse(const clang::TypeLoc typeloc, int depth){
		const auto typeloc2 = typeloc.getAsAdjusted<T>();
		if (!typeloc2.isNull()) TraverseDetail(typeloc2, depth);
	}

	void Traverse(const clang::TypeLoc typeloc, int depth){
		SIMP_DEBUG(debugStr(depth, "TL", typeloc.getType()->getTypeClassName()));
		TestAndTraverse<clang::ConstantArrayTypeLoc>(typeloc, depth+1);
	}

	void TraverseDetail(const clang::ConstantArrayTypeLoc typeloc, int depth){
		Traverse(typeloc.getInnerType(), depth);
		Traverse(typeloc.getSizeExpr(), depth);
	}

	//------------------------------------------------------------------------
	// Types
	//------------------------------------------------------------------------
	void Traverse(const clang::QualType &type, int depth){
		Traverse(type.getTypePtr(), depth);
	}
	void Traverse(const clang::Type *type, int depth){
		if(!type){ return; }
		if(!m_traversed_types.insert(type).second){ return; }

		SIMP_DEBUG(debugStr(depth, "T", type->getTypeClassName()));

		TestAndTraverse<clang::PointerType>(type, depth);
		TestAndTraverse<clang::ReferenceType>(type, depth);
		TestAndTraverse<clang::ConstantArrayType>(type, depth);
		TestAndTraverse<clang::ArrayType>(type, depth);
		TestAndTraverse<clang::AttributedType>(type, depth);
		TestAndTraverse<clang::TypeOfType>(type, depth);
		TestAndTraverse<clang::FunctionProtoType>(type, depth);
		TestAndTraverse<clang::ParenType>(type, depth);
		TestAndTraverse<clang::AutoType>(type, depth);
		TestAndTraverse<clang::DecltypeType>(type, depth);
		TestAndTraverse<clang::RecordType>(type, depth);
		TestAndTraverse<clang::EnumType>(type, depth);
		TestAndTraverse<clang::TypedefType>(type, depth);
		TestAndTraverse<clang::TemplateSpecializationType>(type, depth);
		TestAndTraverse<clang::ElaboratedType>(type, depth);
	}

	void TraverseDetail(const clang::PointerType *type, int depth){
		// dereferenced type
		Traverse(type->getPointeeType(), depth);
	}
	void TraverseDetail(const clang::ReferenceType *type, int depth){
		// dereferenced type
		Traverse(type->getPointeeType(), depth);
	}
	void TraverseDetail(const clang::ArrayType *type, int depth){
		// element type
		Traverse(type->getElementType(), depth);
	}
	void TraverseDetail(const clang::ConstantArrayType *type, int depth){
		// element type
		Traverse(type->getElementType(), depth);
	}
	void TraverseDetail(const clang::AttributedType *type, int depth){
		// qualified type
		Traverse(type->getModifiedType(), depth);
	}
	void TraverseDetail(const clang::TypeOfType *type, int depth){
		Traverse(type->getUnderlyingType(), depth);
	}
	void TraverseDetail(const clang::FunctionProtoType *type, int depth){
		for(const auto paramType : type->param_types()){ Traverse(paramType, depth); }
		Traverse(type->getReturnType(), depth);
	}
	void TraverseDetail(const clang::ParenType *type, int depth){
		Traverse(type->getInnerType(), depth);
	}
	void TraverseDetail(const clang::AutoType *type, int depth){
		// result of type inference
		Traverse(type->getDeducedType(), depth);
	}
	void TraverseDetail(const clang::DecltypeType *type, int depth){
		// formula used for inference
		Traverse(type->getUnderlyingExpr(), depth);
		// result of type inference
		Traverse(type->getUnderlyingType(), depth);
	}
	void TraverseDetail(const clang::RecordType *type, int depth){
		// structure definition
		Traverse(type->getDecl(), depth);
	}
	void TraverseDetail(const clang::EnumType *type, int depth){
		// structure (enum?) definition
		Traverse(type->getDecl(), depth);
	}
	void TraverseDetail(const clang::TypedefType *type, int depth){
		// alias definition
		Traverse(type->getDecl(), depth);
	}
	void TraverseDetail(const clang::TemplateSpecializationType *type, int depth){
		const auto t = type->getTemplateName();
		Traverse(t.getAsTemplateDecl(), depth);
		// template argument
		for(unsigned int i = 0; i < type->getNumArgs(); ++i){
			Traverse(type->getArgs()[i], depth);
		}
	}
	void TraverseDetail(const clang::ElaboratedType *type, int depth){
		Traverse(type->getNamedType(), depth);
		if(type->getQualifier()){
			Traverse(type->getQualifier(), depth);	
		}
	}

	//------------------------------------------------------------------------
	// Others
	//------------------------------------------------------------------------
	void Traverse(const clang::NestedNameSpecifier *spec, int depth){
		switch(spec->getKind()){
			case clang::NestedNameSpecifier::TypeSpec:
			case clang::NestedNameSpecifier::TypeSpecWithTemplate:
				Traverse(spec->getAsType(), depth);
				break;
		}
		if(spec->getPrefix()){
			Traverse(spec->getPrefix(), depth);
		}
	}

	void Traverse(const clang::TemplateArgumentLoc &arg_loc, int depth){
		const auto &arg = arg_loc.getArgument();
		++depth;
		switch(arg.getKind()){
			case clang::TemplateArgument::Type:
				Traverse(arg_loc.getTypeSourceInfo()->getType(), depth);
				break;
			case clang::TemplateArgument::Expression:
				Traverse(arg_loc.getSourceExpression(), depth);
				break;
			case clang::TemplateArgument::Declaration:
				Traverse(arg_loc.getSourceDeclExpression(), depth);
				break;
			case clang::TemplateArgument::NullPtr:
				Traverse(arg_loc.getSourceNullPtrExpression(), depth);
				break;
			case clang::TemplateArgument::Integral:
				Traverse(arg_loc.getSourceIntegralExpression(), depth);
				break;
		}
	}

	void Traverse(const clang::TemplateArgument &arg, int depth){
		++depth;
		switch(arg.getKind()){
			case clang::TemplateArgument::Type:
				Traverse(arg.getAsType(), depth);
				break;
			case clang::TemplateArgument::Expression:
				Traverse(arg.getAsExpr(), depth);
				break;
			case clang::TemplateArgument::Declaration:
				Traverse(arg.getAsDecl(), depth);
				break;
			case clang::TemplateArgument::NullPtr:
				Traverse(arg.getNullPtrType(), depth);
				break;
		}
	}

	//------------------------------------------------------------------------
	// Marking
	//------------------------------------------------------------------------
	template <typename T>
	bool TestAndMark(const clang::Decl *decl, int depth){
		if(clang::isa<T>(decl)){
			return MarkDetail(clang::dyn_cast<T>(decl), depth + 1);
		}else{
			return false;
		}
	}

	void MarkRange(const clang::SourceRange &range){
		MarkRangeSM(range, *m_source_manager, m_marker);
		m_ranges.insert(range);
	}

	void MarkRange(
		const clang::SourceLocation &begin,
		const clang::SourceLocation &end)
	{
		MarkRange(clang::SourceRange(begin, end));
	}

	clang::SourceLocation EndOfHead(const clang::Decl *decl){
		auto min_loc = decl->getEndLoc();
		if(const auto context = clang::dyn_cast<clang::DeclContext>(decl)){
			for(const auto child : context->decls()){
				if(child->isImplicit()){ continue; }
				const auto a = m_source_manager->getPresumedLineNumber(min_loc);
				const auto b = m_source_manager->getPresumedLineNumber(child->getBeginLoc());
				if(b < a){ min_loc = child->getBeginLoc(); }
			}
			const auto a = m_source_manager->getPresumedLineNumber(decl->getBeginLoc());
			const auto b = m_source_manager->getPresumedLineNumber(min_loc);
			if(b > a){ min_loc = PreviousLine(min_loc); }
		}
		return min_loc;
	}

	bool MarkRecursiveFiltered(const clang::Decl *decl, int depth){
		if(m_traversed_decls.find(decl) == m_traversed_decls.end()){
			return false;
		}
		return MarkRecursive(decl, depth);
	}
	bool MarkRecursive(const clang::Decl *decl, int depth){

		SIMP_DEBUG(debug(depth, 'M', decl));

		if(decl->isImplicit()){ return false; }

		bool result = false;
		result |= TestAndMark<clang::AccessSpecDecl>(decl, depth);
		result |= TestAndMark<clang::UsingDirectiveDecl>(decl, depth);
		result |= TestAndMark<clang::NamespaceDecl>(decl, depth);

		result |= TestAndMark<clang::TypedefNameDecl>(decl, depth);
		result |= TestAndMark<clang::TypeAliasTemplateDecl>(decl, depth);
		result |= TestAndMark<clang::RecordDecl>(decl, depth);
		result |= TestAndMark<clang::EnumDecl>(decl, depth);
		result |= TestAndMark<clang::ClassTemplateDecl>(decl, depth);
		result |= TestAndMark<clang::ClassTemplateSpecializationDecl>(decl, depth);

		result |= TestAndMark<clang::FieldDecl>(decl, depth);
		result |= TestAndMark<clang::EnumConstantDecl>(decl, depth);
		result |= TestAndMark<clang::FunctionDecl>(decl, depth);
		result |= TestAndMark<clang::FunctionTemplateDecl>(decl, depth);
		result |= TestAndMark<clang::VarDecl>(decl, depth);
		return result;
	}

	bool MarkDetail(const clang::AccessSpecDecl *decl, int){
		MarkRange(decl->getBeginLoc(), decl->getEndLoc());
		return true;
	}
	bool MarkDetail(const clang::UsingDirectiveDecl *decl, int){
		MarkRange(decl->getBeginLoc(), decl->getEndLoc());
		return true;
	}
	bool MarkDetail(const clang::NamespaceDecl *decl, int depth){
		bool result = false;
		for(const auto child : decl->decls()){
			result |= MarkRecursiveFiltered(child, depth);
		}
		if(result){
			MarkRange(clang::SourceRange(decl->getBeginLoc(), EndOfHead(decl)));
			MarkRange(decl->getRBraceLoc(), decl->getEndLoc());
		}
		return result;
	}

	bool MarkDetail(const clang::TypedefNameDecl *decl, int){
		MarkRange(decl->getBeginLoc(), decl->getEndLoc());
		return true;
	}
	bool MarkDetail(const clang::TypeAliasTemplateDecl *decl, int){
		MarkRange(decl->getBeginLoc(), decl->getEndLoc());
		return true;
	}
	bool MarkDetail(const clang::RecordDecl *decl, int depth){
		MarkRange(clang::SourceRange(decl->getOuterLocStart(), EndOfHead(decl)));
		MarkRange(decl->getBraceRange().getEnd(), decl->getEndLoc());
		for(const auto child : decl->decls()){ MarkRecursiveFiltered(child, depth); }
		return true;
	}
	bool MarkDetail(const clang::EnumDecl *decl, int depth){
		MarkRange(clang::SourceRange(decl->getOuterLocStart(), EndOfHead(decl)));
		MarkRange(decl->getBraceRange().getEnd(), decl->getEndLoc());
		// keeping this unfiltered to preserve enum values
		for(const auto child : decl->decls()){ MarkRecursive(child, depth); }
		return true;
	}
	bool MarkDetail(const clang::ClassTemplateDecl *decl, int depth){
		bool result = MarkRecursive(decl->getTemplatedDecl(), depth);
		for(const auto spec : decl->specializations()){
			result |= MarkRecursive(spec, depth);
		}
		if(result){
			auto template_tail = decl->getTemplatedDecl()->getBeginLoc();
			const auto a = m_source_manager->getPresumedLineNumber(decl->getBeginLoc());
			const auto b = m_source_manager->getPresumedLineNumber(template_tail);
			if(b > a){ template_tail = PreviousLine(template_tail); }
			MarkRange(clang::SourceRange(decl->getBeginLoc(), template_tail));
		}
		return result;
	}
	bool MarkDetail(const clang::ClassTemplateSpecializationDecl *decl, int depth){
		const auto template_head = decl->getSourceRange().getBegin();
		const auto template_tail = decl->getOuterLocStart();
		MarkRange(clang::SourceRange(template_head, template_tail));
		return true;
	}

	bool MarkDetail(const clang::FieldDecl *decl, int depth){
		MarkRange(decl->getBeginLoc(), decl->getEndLoc());
		return true;
	}
	bool MarkDetail(const clang::EnumConstantDecl *decl, int depth){
		MarkRange(decl->getBeginLoc(), decl->getEndLoc());
		return true;
	}
	bool MarkDetail(const clang::FunctionDecl *decl, int depth){
		MarkRange(decl->getBeginLoc(), decl->getEndLoc());
		return true;
	}
	bool MarkDetail(const clang::FunctionTemplateDecl *decl, int depth){
		bool result = MarkRecursive(decl->getTemplatedDecl(), depth);
		for(const auto spec : decl->specializations()){
			result |= MarkRecursive(spec, depth);
		}
		if(result){
			auto template_tail = decl->getTemplatedDecl()->getBeginLoc();
			const auto a = m_source_manager->getPresumedLineNumber(decl->getBeginLoc());
			const auto b = m_source_manager->getPresumedLineNumber(template_tail);
			if(b > a){ template_tail = PreviousLine(template_tail); }
			MarkRange(clang::SourceRange(decl->getBeginLoc(), template_tail));
		}
		return result;
	}
	bool MarkDetail(const clang::VarDecl *decl, int depth){
		MarkRange(decl->getBeginLoc(), decl->getEndLoc());
		return true;
	}

public:

	ASTConsumer(std::shared_ptr<ReachabilityMarker> marker
		, const std::unordered_set<std::string>& roots
		, SourceRangeSet& ranges)
		: clang::ASTConsumer()
		, m_marker(std::move(marker))
		, m_roots(roots)
		, m_ranges(ranges)
	{ }

	std::unordered_set<const clang::FunctionDecl*> getFuncRoots(
		const clang::TranslationUnitDecl* tu,
		const clang::IdentifierTable& idents){
		std::unordered_set<const clang::FunctionDecl*> result;
		for(const auto root : m_roots){
			bool inserted = false;
			if (const auto maybeFound = idents.find(root);
				maybeFound != idents.end()) {
				const auto lookupResult = tu->lookup(clang::DeclarationName(maybeFound->getValue()));
				for (const auto named_decl : lookupResult){
					if(const auto func_decl = named_decl->getAsFunction()){
						result.insert(func_decl);
						inserted = true;
					}
				}
			}
			if (!inserted){
				// TODO earlier checking, not throwing runtime_error
				std::ostringstream oss;
				oss << "Error: root '" <<  root << "' not found." << std::endl;
				throw std::runtime_error(oss.str());
			}
		}
		return result;
	}

	bool IsBefore(const clang::Decl* prev, const clang::Decl* curr){
		const auto lhs = m_source_manager->getPresumedLoc(prev->getEndLoc());
		const auto rhs = m_source_manager->getPresumedLoc(curr->getBeginLoc());

		// only care about declarations from a source file
		// that too the same source file
		if (lhs.isInvalid()
			|| rhs.isInvalid()
			|| lhs.getFileID() != rhs.getFileID())
			return true;

		// easy case
		if  (lhs.getLine() < rhs.getLine())
			return true;

		// declaration inside a macro
		const auto prev_begin = m_source_manager->getPresumedLoc(prev->getBeginLoc());
		const auto prev_end = lhs;
		const auto curr_begin = rhs;
		const auto curr_end = m_source_manager->getPresumedLoc(curr->getEndLoc());
		const auto eq = [](const clang::PresumedLoc& a, const clang::PresumedLoc& b){
			return a.getLine() == b.getLine() && a.getColumn() == b.getColumn();
		};
		if (prev->getBeginLoc().isMacroID()
			&& curr->getBeginLoc().isMacroID()
			&& eq(prev_begin, prev_end)
			&& eq(prev_end, curr_begin)
			&& eq(curr_begin, curr_end))
				return true;

		// typedef on struct definition
		if (clang::dyn_cast<clang::RecordDecl>(prev)
			&& clang::isa<clang::TypedefNameDecl>(curr)
			&& curr->getSourceRange().fullyContains(prev->getSourceRange())){
				return true;
		}

		if (clang::isa<clang::EmptyDecl>(curr)){
			return true;
		}

		if (const auto func = clang::dyn_cast<clang::FunctionDecl>(curr)){
			if (0 != func->getBuiltinID()){
				return true;
			}
		}

		// extern {struct tag, enum} var(args);
		if ((clang::isa<clang::RecordDecl>(prev) || clang::isa<clang::EnumDecl>(prev))
			&& (clang::isa<clang::VarDecl>(curr) || clang::isa<clang::FunctionDecl>(curr))
			&& curr->getSourceRange().fullyContains(prev->getSourceRange())){
			return true;
		}

		// multiple vars in one-line declarator: `T a, b;`
		if (clang::isa<clang::VarDecl>(curr)
			&& clang::isa<clang::VarDecl>(prev)
			&& prev_begin.getLine() == prev_end.getLine()
			&& prev_end.getLine() == curr_begin.getLine()
			&& curr_begin.getLine() == curr_end.getLine()){
				return true;
		}

		return false;
	}

	bool ToplevelOnDiffLines(const clang::TranslationUnitDecl* tu,
		clang::DiagnosticsEngine& diagnostics, const unsigned not_after_id){
		auto it = tu->decls_begin();
		const auto end = tu->decls_end();
		const auto name_of = [](auto* decl){
			if(const auto named_decl = clang::dyn_cast<clang::NamedDecl>(decl)){
				return named_decl->getNameAsString();
			}else{
				return std::string();
			}
		};
		
                auto result = true;

		// empty case
		if(it == end) return result;
		
		// non-empty case
		auto prev = *it;
		while (++it != end){
			const auto curr = *it;
			if(!IsBefore(prev, curr)) {
				result = false;
				diagnostics.Report(curr->getBeginLoc(), not_after_id)
					<< name_of(prev)
					<< name_of(curr);
			}
			prev = curr;
		}


		return result;
	}

	virtual void HandleTranslationUnit(clang::ASTContext &context) override {
		const auto &sm = context.getSourceManager();
		const auto tu = context.getTranslationUnitDecl();
		auto& diagnostics = context.getDiagnostics();
		const auto not_after_id = diagnostics.getCustomDiagID(
			clang::DiagnosticsEngine::Error,
			"declaration '%1' must start on a line after the end of declaration '%0'");

		//SIMP_DEBUG(tu->dump());
		m_source_manager = &sm;
		reset();

		if(!ToplevelOnDiffLines(tu, diagnostics, not_after_id))
			return;

		if (!m_roots.empty()){
			const auto funcRoots = getFuncRoots(tu, context.Idents);
			for (const auto decl : funcRoots) Traverse(decl, 0);
		}else{
			for(const auto decl : tu->decls()){
				if(const auto func_decl = clang::dyn_cast<clang::FunctionDecl>(decl)){
					if(func_decl->isMain()){
						Traverse(decl, 0);
					}
				}
			}
		}

		for(const auto decl : tu->decls()){
			if (m_traversed_decls.find(decl) != m_traversed_decls.end()
				|| isAFwdDeclWhoseDefIsTraversed(decl))
				MarkRecursive(decl, 0);
		}
	}

	// TODO This can't be the best/correct way...
	bool isAFwdDeclWhoseDefIsTraversed(const clang::Decl *decl){
		if(const auto tagDecl = clang::dyn_cast<clang::TagDecl>(decl)){
			if (const auto maybeDef = tagDecl->getDefinition()){
				if (m_traversed_decls.find(maybeDef) != m_traversed_decls.end()){
					return true;
				}
			}
		}
		return false;
	}

};


ReachabilityAnalyzer::ReachabilityAnalyzer(
	std::shared_ptr<ReachabilityMarker> marker,
	const std::unordered_set<std::string>& roots)
	: clang::ASTFrontendAction()
	, m_marker(std::move(marker))
	, m_roots(roots)
	, m_ranges()
{ }

std::unique_ptr<clang::ASTConsumer> ReachabilityAnalyzer::CreateASTConsumer(
	clang::CompilerInstance &ci,
	llvm::StringRef in_file)
{
	return std::make_unique<ReachabilityAnalyzer::ASTConsumer>(m_marker, m_roots, m_ranges);
}

bool actualFile(clang::SourceRange defn_range, const clang::SourceManager& sm){
	const auto presumed = sm.getPresumedLoc(defn_range.getBegin());
	assert(presumed.isValid());
	auto result = std::strcmp(presumed.getFilename(), "<built-in>") != 0
		&& std::strcmp(presumed.getFilename(), "<command line>") != 0;
	if (!result) SIMP_DEBUG(debugStr(2, "Not from a file", ""));
	return result;
}

void ReachabilityAnalyzer::findMacroDefns(clang::PreprocessingRecord& pr, const clang::SourceManager& sm
	, clang::SourceRange range, const PPRecordNested::RangeToSet& macro_deps
	, const PPRecordNested::RangeToRange& range_hack, SourceRangeSet& marked){

	SIMP_DEBUG(debugStr(0, "Macro expansions in " + RangeToString(range, sm), ""));
	const auto inRange = pr.getPreprocessedEntitiesInRange(range);
	for (const auto* entity : inRange) {
		assert(entity);
		if(const auto macro_exp = clang::dyn_cast<clang::MacroExpansion>(entity)){
			SIMP_DEBUG(debugStr(1, macro_exp->getName()->getNameStart(), ""));
			SIMP_DEBUG(debugStr(2, "Expanded at " + RangeToString(macro_exp->getSourceRange(), sm), ""));
			const auto defn_range = macro_exp->getDefinition()->getSourceRange();
			SIMP_DEBUG(debugStr(2, "Defined " + RangeToString(defn_range, sm), ""));

			// Not built-in, from command line, nor already marked
			if (actualFile(defn_range, sm) && marked.find(defn_range) == marked.end()){
				MarkRangeDSM(2, range_hack.at(defn_range), sm, m_marker);
				marked.emplace(defn_range);
			}

			const auto dep_ranges = macro_deps.find(defn_range);
			// skip if macro has no dependencies
			if (dep_ranges == macro_deps.end()) continue;

			// mark all macros used by this macro
			for (const auto& dep_range : dep_ranges->second){
				SIMP_DEBUG(debugStr(2, "Uses " + RangeToString(dep_range, sm), ""));
				if (marked.find(dep_range) == marked.end()) {
					MarkRangeDSM(3, range_hack.at(dep_range), sm, m_marker);
					marked.emplace(dep_range);
				}
			}
		}
	}
}

// 1. The PreprocessingRecord only inlcudes the Macro expanded when typed into the source,
//    not any subsequent macros expanded _inside a macro_.
// 2. None of SourceManager::{getFileLoc,getExpansionLoc,get(Immediate)SpellingLoc}() enable 
//    to go from the source _down_/expand the Macros, only to go from a fully expanded location
//    back up to the source.
// 3. It seems difficult/not sensible to re-lex/pp the macro _definition_ to get at the macros
//    it refers to.
// Therefore, we add a PPCallback that records _all_ macro expansions, even those that occur
// within a macro.
void ReachabilityAnalyzer::ExecuteAction()
{
	auto &ci = getCompilerInstance();
	auto &pp = ci.getPreprocessor();
	PPRecordNested::RangeToSet macro_deps;
	PPRecordNested::RangeToRange range_hack;
	const auto &sm = ci.getSourceManager();
	pp.addPPCallbacks(std::make_unique<PPRecordNested>(macro_deps, range_hack, sm));

	// this runs the pre-processor and declaration traversal & marking
	ASTFrontendAction::ExecuteAction();

	// you need this for `PreprocessingRecord::getPreprocessedEntitiesInRange`
	auto *pr = pp.getPreprocessingRecord();
	if (!pr) {
		SIMP_DEBUG(std::cerr << "No preprocessing record found :(" << std::endl);
		return;
	}

	// mark the marcros used inside all declaration ranges
	SourceRangeSet marked;
	for (const auto& range : m_ranges)
		findMacroDefns(*pr, sm, range, macro_deps, range_hack, marked);
}


ReachabilityAnalyzerFactory::ReachabilityAnalyzerFactory(
	std::shared_ptr<ReachabilityMarker> marker,
	const std::unordered_set<std::string>& roots)
	: clang::tooling::FrontendActionFactory()
	, m_marker(std::move(marker))
	, m_roots(roots)
{ }

std::unique_ptr<clang::FrontendAction> ReachabilityAnalyzerFactory::create(){
	return std::make_unique<ReachabilityAnalyzer>(m_marker, m_roots);
}

