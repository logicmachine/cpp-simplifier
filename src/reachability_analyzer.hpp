#ifndef CPP_SIMPLIFIER_REACHABILITY_ANALYZER_HPP
#define CPP_SIMPLIFIER_REACHABILITY_ANALYZER_HPP

#include <memory>
#include <unordered_set>
#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include "reachability_marker.hpp"

class ReachabilityAnalyzer : public clang::ASTFrontendAction {

private:
	std::shared_ptr<ReachabilityMarker> m_marker;
	const std::unordered_set<std::string>& m_roots;

public:
	class ASTConsumer;

	ReachabilityAnalyzer(
		std::shared_ptr<ReachabilityMarker> marker,
		const std::unordered_set<std::string>& roots);

	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance &ci,
		llvm::StringRef in_file) override;
        
        
};

class ReachabilityAnalyzerFactory
	: public clang::tooling::FrontendActionFactory
{

private:
	std::shared_ptr<ReachabilityMarker> m_marker;
	const std::unordered_set<std::string>& m_roots;

public:
	ReachabilityAnalyzerFactory(
		std::shared_ptr<ReachabilityMarker> marker,
		const std::unordered_set<std::string>& roots);

	virtual std::unique_ptr<clang::FrontendAction> create() override;

};

#endif

