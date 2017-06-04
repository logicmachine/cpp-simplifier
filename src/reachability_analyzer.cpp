#include <iostream>
#include <unordered_set>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include "reachability_analyzer.hpp"

class ReachabilityAnalyzer::ASTConsumer : public clang::ASTConsumer {

private:
	const clang::SourceManager *m_source_manager;
	std::unordered_set<const clang::Decl *> m_traversed_decls;
	std::unordered_set<const clang::Stmt *> m_traversed_stmts;
	std::unordered_set<const clang::Type *> m_traversed_types;

	std::shared_ptr<ReachabilityMarker> m_marker;

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

	//------------------------------------------------------------------------
	// Declarations
	//------------------------------------------------------------------------
	void Traverse(const clang::Decl *decl, int depth){
		if(!decl){ return; }
		if(!m_traversed_decls.insert(decl).second){ return; }
#ifdef DEBUG_DUMP_AST
		std::cerr << std::string(depth * 2, ' ')
		          << "D: " << decl->getDeclKindName();
		if(clang::isa<clang::NamedDecl>(decl)){
			const auto named_decl = clang::dyn_cast<clang::NamedDecl>(decl);
			const auto name = named_decl->getNameAsString();
			std::cerr << " (" << name << ")" << std::endl;
		}else{
			std::cerr << std::endl;
		}
		{
			const auto range = decl->getSourceRange();
			std::cerr << std::string(depth * 2 + 4, ' ') << "- "
			          << range.getBegin().printToString(*m_source_manager)
			          << std::endl;
			std::cerr << std::string(depth * 2 + 4, ' ') << "- "
			          << range.getEnd().printToString(*m_source_manager)
			          << std::endl;
		}
#endif
		{	// 親の定義
			const auto ctx = decl->getDeclContext();
			if(ctx && clang::isa<clang::Decl>(ctx)){
				Traverse(clang::dyn_cast<clang::Decl>(ctx), depth + 1);
			}
		}

		TestAndTraverse<clang::NamespaceDecl>(decl, depth);

		TestAndTraverse<clang::TypedefNameDecl>(decl, depth);
		TestAndTraverse<clang::CXXRecordDecl>(decl, depth);
		TestAndTraverse<clang::ClassTemplateSpecializationDecl>(decl, depth);
		TestAndTraverse<clang::TemplateDecl>(decl, depth);
		TestAndTraverse<clang::ClassTemplateDecl>(decl, depth);

		TestAndTraverse<clang::ValueDecl>(decl, depth);
		TestAndTraverse<clang::FieldDecl>(decl, depth);
		TestAndTraverse<clang::FunctionDecl>(decl, depth);
		TestAndTraverse<clang::CXXConstructorDecl>(decl, depth);
		TestAndTraverse<clang::VarDecl>(decl, depth);
		TestAndTraverse<clang::ParmVarDecl>(decl, depth);
	}

	void TraverseDetail(const clang::NamespaceDecl *decl, int depth){
		for(const auto child : decl->decls()){
			// グローバル変数
			if(clang::isa<clang::VarDecl>(child)){ Traverse(child, depth); }
		}
	}

	void TraverseDetail(const clang::TypedefNameDecl *decl, int depth){
		// 別名をつけられた型
		Traverse(decl->getUnderlyingType(), depth);
	}
	void TraverseDetail(const clang::CXXRecordDecl *decl, int depth){
		if(!decl->hasDefinition()){
			Traverse(decl->getDefinition(), depth);
			return;
		}
		for(const auto child : decl->decls()){
			// アクセス指定子
			if(clang::isa<clang::AccessSpecDecl>(child)){ Traverse(child, depth); }
		}
		// 基底クラス
		for(const auto &base : decl->bases()){ Traverse(base.getType(), depth); }
		// 仮想基底クラス
		for(const auto &vbase : decl->vbases()){ Traverse(vbase.getType(), depth); }
		// ユーザ定義デストラクタ
		if(decl->hasUserDeclaredDestructor()){
			Traverse(decl->getDestructor(), depth);
		}
	}
	void TraverseDetail(const clang::ClassTemplateSpecializationDecl *decl, int depth){
		// 特殊化元クラステンプレート
		Traverse(decl->getSpecializedTemplate(), depth);
	}
	void TraverseDetail(const clang::TemplateDecl *decl, int depth){
		// テンプレート引数
		for(const auto param : *(decl->getTemplateParameters())){
			Traverse(param, depth);
		}
	}
	void TraverseDetail(const clang::ClassTemplateDecl *decl, int depth){
		// 特殊化元レコード型
		Traverse(decl->getTemplatedDecl(), depth);
	}

	void TraverseDetail(const clang::ValueDecl *decl, int depth){
		// 対応する型
		Traverse(decl->getType(), depth);
	}
	void TraverseDetail(const clang::FieldDecl *decl, int depth){
		// ビットフィールドの幅
		if(decl->isBitField()){ Traverse(decl->getBitWidth(), depth); }
		// メンバ初期化子
		if(decl->hasInClassInitializer()){
			Traverse(decl->getInClassInitializer(), depth);
		}
	}
	void TraverseDetail(const clang::FunctionDecl *decl, int depth){
		// テンプレート関数の特殊化元
		if(decl->isFunctionTemplateSpecialization()){
			Traverse(decl->getPrimaryTemplate(), depth);
		}
		// 引数
		for(const auto param : decl->params()){ Traverse(param, depth); }
		// 処理内容
		if(decl->hasBody()){ Traverse(decl->getBody(), depth); }
		// 戻り値の型
		Traverse(decl->getReturnType(), depth);
	}
	void TraverseDetail(const clang::CXXConstructorDecl *decl, int depth){
		// 初期化リスト
		for(const auto init : decl->inits()){
			if(init->getMember()){ Traverse(init->getMember(), depth); }
			Traverse(init->getInit(), depth);
		}
	}
	void TraverseDetail(const clang::VarDecl *decl, int depth){
		// 初期化式
		if(decl->hasInit()){ Traverse(decl->getInit(), depth); }
		// 型情報
		if(decl->getTypeSourceInfo()){
			Traverse(decl->getTypeSourceInfo()->getType(), depth);
		}
	}
	void TraverseDetail(const clang::ParmVarDecl *decl, int depth){
		// デフォルト引数
		if(decl->hasDefaultArg()){ Traverse(decl->getDefaultArg(), depth); }
	}

	//------------------------------------------------------------------------
	// Statements
	//------------------------------------------------------------------------
	void Traverse(const clang::Stmt *stmt, int depth){
		if(!stmt){ return; }
		if(!m_traversed_stmts.insert(stmt).second){ return; }
#ifdef DEBUG_DUMP_AST
		std::cerr << std::string(depth * 2, ' ')
		          << "S: " << stmt->getStmtClassName() << std::endl;
#endif
		for(const auto child : stmt->children()){
			Traverse(child, depth + 1);
		}
		TestAndTraverse<clang::DeclStmt>(stmt, depth);
		TestAndTraverse<clang::DeclRefExpr>(stmt, depth);
		TestAndTraverse<clang::MemberExpr>(stmt, depth);
		TestAndTraverse<clang::CallExpr>(stmt, depth);
		TestAndTraverse<clang::CXXConstructExpr>(stmt, depth);
		TestAndTraverse<clang::ExplicitCastExpr>(stmt, depth);
		TestAndTraverse<clang::UnaryExprOrTypeTraitExpr>(stmt, depth);
	}

	void TraverseDetail(const clang::DeclStmt *stmt, int depth){
		// 子として含まれている定義すべて
		for(const auto decl : stmt->decls()){ Traverse(decl, depth); }
	}
	void TraverseDetail(const clang::DeclRefExpr *expr, int depth){
		// 参照が指している定義
		Traverse(expr->getDecl(), depth);
		// テンプレート引数
		for(unsigned int i = 0; i < expr->getNumTemplateArgs(); ++i){
			Traverse(expr->getTemplateArgs()[i], depth);
		}
		if(expr->hasQualifier()){
			Traverse(expr->getQualifier(), depth);
		}
	}
	void TraverseDetail(const clang::MemberExpr *expr, int depth){
		// メンバの定義
		Traverse(expr->getMemberDecl(), depth);
		// テンプレート引数
		for(unsigned int i = 0; i < expr->getNumTemplateArgs(); ++i){
			Traverse(expr->getTemplateArgs()[i], depth);
		}
	}
	void TraverseDetail(const clang::CallExpr *expr, int depth){
		// 呼び出される関数
		Traverse(expr->getCallee(), depth);
	}
	void TraverseDetail(const clang::CXXConstructExpr *expr, int depth){
		// コンストラクタの定義
		Traverse(expr->getConstructor(), depth);
	}
	void TraverseDetail(const clang::ExplicitCastExpr *expr, int depth){
		// キャスト先の型
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
	// Types
	//------------------------------------------------------------------------
	void Traverse(const clang::QualType &type, int depth){
		Traverse(type.getTypePtrOrNull(), depth);
	}
	void Traverse(const clang::Type *type, int depth){
		if(!type){ return; }
		if(!m_traversed_types.insert(type).second){ return; }
#ifdef DEBUG_DUMP_AST
		std::cerr << std::string(depth * 2, ' ')
		          << "T: " << type->getTypeClassName() << std::endl;
#endif
		TestAndTraverse<clang::PointerType>(type, depth);
		TestAndTraverse<clang::ReferenceType>(type, depth);
		TestAndTraverse<clang::ArrayType>(type, depth);
		TestAndTraverse<clang::AttributedType>(type, depth);
		TestAndTraverse<clang::AutoType>(type, depth);
		TestAndTraverse<clang::DecltypeType>(type, depth);
		TestAndTraverse<clang::RecordType>(type, depth);
		TestAndTraverse<clang::TypedefType>(type, depth);
		TestAndTraverse<clang::TemplateSpecializationType>(type, depth);
		TestAndTraverse<clang::ElaboratedType>(type, depth);
	}

	void TraverseDetail(const clang::PointerType *type, int depth){
		// デリファレンスした型
		Traverse(type->getPointeeType(), depth);
	}
	void TraverseDetail(const clang::ReferenceType *type, int depth){
		// デリファレンスした型
		Traverse(type->getPointeeType(), depth);
	}
	void TraverseDetail(const clang::ArrayType *type, int depth){
		// 要素の型
		Traverse(type->getElementType(), depth);
	}
	void TraverseDetail(const clang::AttributedType *type, int depth){
		// 修飾された型
		Traverse(type->getModifiedType(), depth);
	}
	void TraverseDetail(const clang::AutoType *type, int depth){
		// 型推論の結果
		Traverse(type->getDeducedType(), depth);
	}
	void TraverseDetail(const clang::DecltypeType *type, int depth){
		// 推論に使用した式
		Traverse(type->getUnderlyingExpr(), depth);
		// 型推論の結果
		Traverse(type->getUnderlyingType(), depth);
	}
	void TraverseDetail(const clang::RecordType *type, int depth){
		// 構造体の定義
		Traverse(type->getDecl(), depth);
	}
	void TraverseDetail(const clang::TypedefType *type, int depth){
		// 別名の定義
		Traverse(type->getDecl(), depth);
	}
	void TraverseDetail(const clang::TemplateSpecializationType *type, int depth){
		const auto t = type->getTemplateName();
		Traverse(t.getAsTemplateDecl(), depth);
		// テンプレート引数
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
		const auto main_file_id = m_source_manager->getMainFileID();
		const auto begin = range.getBegin();
		const auto end = range.getEnd();
		if(m_source_manager->getFileID(begin) != main_file_id){ return; }
		const auto begin_line =
			m_source_manager->getPresumedLineNumber(begin) - 1;
		const auto end_line =
			m_source_manager->getPresumedLineNumber(end) - 1;
		for(unsigned int i = begin_line; i <= end_line; ++i){
			m_marker->mark(i);
		}
	}

	clang::SourceLocation EndOfHead(const clang::Decl *decl){
		auto loc = decl->getLocEnd();
		if(clang::isa<clang::DeclContext>(decl)){
			const auto context = clang::dyn_cast<clang::DeclContext>(decl);
			for(const auto child : context->decls()){
				if(child->isImplicit()){ continue; }
				const auto a =
					m_source_manager->getPresumedLineNumber(loc);
				const auto b =
					m_source_manager->getPresumedLineNumber(child->getLocStart());
				if(b < a){ loc = child->getLocStart(); }
			}
			const auto a =
				m_source_manager->getPresumedLineNumber(decl->getLocStart());
			const auto b = m_source_manager->getPresumedLineNumber(loc);
			if(b > a){ loc = PreviousLine(loc); }
		}
		return loc;
	}

	bool MarkRecursive(const clang::Decl *decl, int depth){
		if(m_traversed_decls.find(decl) == m_traversed_decls.end()){
			return false;
		}
#ifdef DEBUG_DUMP_AST
		std::cerr << std::string(depth * 2, ' ')
		          << "M: " << decl->getDeclKindName();
		if(clang::isa<clang::NamedDecl>(decl)){
			const auto named_decl = clang::dyn_cast<clang::NamedDecl>(decl);
			const auto name = named_decl->getNameAsString();
			std::cerr << " (" << name << ")" << std::endl;
		}else{
			std::cerr << std::endl;
		}
		{
			const auto range = decl->getSourceRange();
			std::cerr << std::string(depth * 2 + 4, ' ') << "- "
			          << range.getBegin().printToString(*m_source_manager)
			          << std::endl;
			std::cerr << std::string(depth * 2 + 4, ' ') << "- "
			          << range.getEnd().printToString(*m_source_manager)
			          << std::endl;
		}
#endif
		bool result = false;
		result |= TestAndMark<clang::AccessSpecDecl>(decl, depth);
		result |= TestAndMark<clang::UsingDirectiveDecl>(decl, depth);
		result |= TestAndMark<clang::NamespaceDecl>(decl, depth);

		result |= TestAndMark<clang::TypedefDecl>(decl, depth);
		result |= TestAndMark<clang::TypeAliasDecl>(decl, depth);
		result |= TestAndMark<clang::TypeAliasTemplateDecl>(decl, depth);
		result |= TestAndMark<clang::RecordDecl>(decl, depth);
		result |= TestAndMark<clang::ClassTemplateDecl>(decl, depth);
		result |= TestAndMark<clang::ClassTemplateSpecializationDecl>(decl, depth);

		result |= TestAndMark<clang::FieldDecl>(decl, depth);
		result |= TestAndMark<clang::FunctionDecl>(decl, depth);
		result |= TestAndMark<clang::FunctionTemplateDecl>(decl, depth);
		result |= TestAndMark<clang::VarDecl>(decl, depth);
		return result;
	}

	bool MarkDetail(const clang::AccessSpecDecl *decl, int){
		MarkRange(decl->getSourceRange());
		return true;
	}
	bool MarkDetail(const clang::UsingDirectiveDecl *decl, int){
		MarkRange(decl->getSourceRange());
		return true;
	}
	bool MarkDetail(const clang::NamespaceDecl *decl, int depth){
		bool result = false;
		for(const auto child : decl->decls()){
			result |= MarkRecursive(child, depth);
		}
		if(result){
			MarkRange(clang::SourceRange(decl->getLocStart(), EndOfHead(decl)));
			MarkRange(clang::SourceRange(decl->getRBraceLoc()));
		}
		return result;
	}

	bool MarkDetail(const clang::TypedefDecl *decl, int){
		MarkRange(decl->getSourceRange());
		return true;
	}
	bool MarkDetail(const clang::TypeAliasDecl *decl, int){
		MarkRange(decl->getSourceRange());
		return true;
	}
	bool MarkDetail(const clang::TypeAliasTemplateDecl *decl, int){
		MarkRange(decl->getSourceRange());
		return true;
	}
	bool MarkDetail(const clang::RecordDecl *decl, int depth){
		MarkRange(clang::SourceRange(decl->getOuterLocStart(), EndOfHead(decl)));
		MarkRange(clang::SourceRange(decl->getRBraceLoc()));
		for(const auto child : decl->decls()){ MarkRecursive(child, depth); }
		return true;
	}
	bool MarkDetail(const clang::ClassTemplateDecl *decl, int depth){
		bool result = MarkRecursive(decl->getTemplatedDecl(), depth);
		for(const auto spec : decl->specializations()){
			result |= MarkRecursive(spec, depth);
		}
		if(result){
			auto template_tail = decl->getTemplatedDecl()->getLocStart();
			const auto a = m_source_manager->getPresumedLineNumber(decl->getLocStart());
			const auto b = m_source_manager->getPresumedLineNumber(template_tail);
			if(b > a){ template_tail = PreviousLine(template_tail); }
			MarkRange(clang::SourceRange(decl->getLocStart(), template_tail));
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
		MarkRange(decl->getSourceRange());
		return true;
	}
	bool MarkDetail(const clang::FunctionDecl *decl, int depth){
		MarkRange(decl->getSourceRange());
		return true;
	}
	bool MarkDetail(const clang::FunctionTemplateDecl *decl, int depth){
		bool result = MarkRecursive(decl->getTemplatedDecl(), depth);
		for(const auto spec : decl->specializations()){
			result |= MarkRecursive(spec, depth);
		}
		if(result){
			auto template_tail = decl->getTemplatedDecl()->getLocStart();
			const auto a = m_source_manager->getPresumedLineNumber(decl->getLocStart());
			const auto b = m_source_manager->getPresumedLineNumber(template_tail);
			if(b > a){ template_tail = PreviousLine(template_tail); }
			MarkRange(clang::SourceRange(decl->getLocStart(), template_tail));
		}
		return result;
	}
	bool MarkDetail(const clang::VarDecl *decl, int depth){
		MarkRange(decl->getSourceRange());
		return true;
	}

public:
	ASTConsumer(std::shared_ptr<ReachabilityMarker> marker)
		: clang::ASTConsumer()
		, m_marker(std::move(marker))
	{ }

	virtual void HandleTranslationUnit(clang::ASTContext &context) override {
		const auto &sm = context.getSourceManager();
		const auto tu = context.getTranslationUnitDecl();
#ifdef DEBUG_DUMP_AST
		tu->dump();
#endif
		m_source_manager = &sm;
		reset();
		for(const auto decl : tu->decls()){
			if(clang::isa<clang::VarDecl>(decl)){
				Traverse(decl, 0);
			}else if(clang::isa<clang::NamespaceDecl>(decl)){
				Traverse(decl, 0);
			}else if(clang::isa<clang::UsingDirectiveDecl>(decl)){
				Traverse(decl, 0);
			}else if(clang::isa<clang::FunctionDecl>(decl)){
				const auto func_decl =
					clang::dyn_cast<clang::FunctionDecl>(decl);
				if(func_decl->isMain()){
					Traverse(decl, 0);
				}
			}
		}
		for(const auto decl : tu->decls()){
			MarkRecursive(decl, 0);
		}
	}

};


ReachabilityAnalyzer::ReachabilityAnalyzer(
	std::shared_ptr<ReachabilityMarker> marker)
	: clang::ASTFrontendAction()
	, m_marker(std::move(marker))
{ }

std::unique_ptr<clang::ASTConsumer> ReachabilityAnalyzer::CreateASTConsumer(
	clang::CompilerInstance &ci,
	llvm::StringRef in_file)
{
	return std::make_unique<ASTConsumer>(m_marker);
}


ReachabilityAnalyzerFactory::ReachabilityAnalyzerFactory(
	std::shared_ptr<ReachabilityMarker> marker)
	: clang::tooling::FrontendActionFactory()
	, m_marker(std::move(marker))
{ }

clang::FrontendAction *ReachabilityAnalyzerFactory::create(){
	return new ReachabilityAnalyzer(m_marker);
}

