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

#ifndef CPP_SIMPLIFIER_REACHABILITY_ANALYZER_HPP
#define CPP_SIMPLIFIER_REACHABILITY_ANALYZER_HPP

#include <memory>
#include <unordered_set>
#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include "debug.hpp"
#include "reachability_marker.hpp"

struct SourceRangeHash {
	size_t operator()(const clang::SourceRange &range) const
	{
		size_t result = std::hash<unsigned>()(range.getBegin().getRawEncoding());
		result <<= sizeof(unsigned) * 8;
		result |= std::hash<unsigned>()(range.getEnd().getRawEncoding());
		return result;
	}
};

using SourceRangeSet = std::unordered_set<clang::SourceRange, SourceRangeHash>;

class PPRecordNested : public clang::PPCallbacks {
public:
	using Map = std::unordered_map<clang::SourceRange, SourceRangeSet, SourceRangeHash>;
private:
	Map& m_macro_deps;
	clang::SourceManager &m_sm;
public:
        clang::SourceRange defn_range(const clang::MacroDefinition& MD) {
		auto macro_info= MD.getMacroInfo();
		assert (macro_info);
		return clang::SourceRange(macro_info->getDefinitionLoc(), macro_info->getDefinitionEndLoc());
	}

	virtual void MacroExpands(
		const clang::Token &Id
		, const clang::MacroDefinition &MD
		, clang::SourceRange Range
		, const clang::MacroArgs *Args) override
	{
		static bool assigned = false;
		static clang::SourceRange outer_macro_defn_range;
		if (!Id.getLocation().isMacroID()) {
			outer_macro_defn_range = defn_range(MD);
			assigned = true;
			SIMP_DEBUG(std::cerr << "TL Macro: " << Id.getIdentifierInfo()->getNameStart() << std::endl);
		} else {
			assert (assigned);
			int count = 0;
			for (auto loc = Id.getLocation(); loc.isMacroID(); loc = m_sm.getImmediateMacroCallerLoc(loc), count++)
			{}
			SIMP_DEBUG(std::cerr << std::string(count * 2, ' ') << Id.getIdentifierInfo()->getNameStart() << std::endl);
			m_macro_deps[outer_macro_defn_range].emplace(defn_range(MD));
		}
	}
	PPRecordNested(Map& macro_deps
		, clang::SourceManager& sm)
		: clang::PPCallbacks()
		, m_macro_deps(macro_deps)
		, m_sm(sm)
	{}
	virtual ~PPRecordNested() { }
};

class ReachabilityAnalyzer : public clang::ASTFrontendAction {

private:
	std::shared_ptr<ReachabilityMarker> m_marker;
	SourceRangeSet m_ranges;
	const std::unordered_set<std::string>& m_roots;

protected:
	void ExecuteAction() override;

	void findMacroDefns(
		clang::PreprocessingRecord& pr,
		clang::SourceManager& sm,
		clang::SourceRange range,
		const PPRecordNested::Map& macro_deps,
		SourceRangeSet& marked);
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

