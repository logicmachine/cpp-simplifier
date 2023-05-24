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

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <clang/Basic/FileManager.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Lex/Preprocessor.h>
#include "inclusion_unroller.hpp"

class InclusionUnrollingAction
	: public clang::PreprocessorFrontendAction
{

private:
	class InclusionHandler : public clang::PPCallbacks {
	private:
		InclusionUnrollingAction *m_action;
	public:
		explicit InclusionHandler(
			InclusionUnrollingAction *action)
			: m_action(action)
		{ }
		virtual void FileChanged(
			clang::SourceLocation loc,
			FileChangeReason,
			clang::SrcMgr::CharacteristicKind,
			clang::FileID) override
		{
			m_action->OnFileChanged(loc);
		}
		virtual void InclusionDirective(
			clang::SourceLocation hash_loc,
			const clang::Token &,
			clang::StringRef filename,
			bool is_angled,
			clang::CharSourceRange,
			const clang::FileEntry *file,
			clang::StringRef,
			clang::StringRef,
			const clang::Module *,
			clang::SrcMgr::CharacteristicKind) override
		{
			m_action->OnInclusionDirective(
				hash_loc, filename, is_angled, file);
		}
	};

	std::shared_ptr<std::string> m_result_ptr;
	std::string m_input_content;
	std::string m_input_filename;

	clang::SourceManager *m_current_source_manager;

	std::unordered_map<std::string, std::vector<std::string>> m_source_cache;
	std::vector<std::string> *m_current_source;

	std::unordered_set<std::string> m_angled_inclusions;

public:
	InclusionUnrollingAction(
		std::shared_ptr<std::string> result_ptr,
		std::string input_content,
		std::string input_filename)
		: clang::PreprocessorFrontendAction()
		, m_result_ptr(std::move(result_ptr))
		, m_input_content(std::move(input_content))
		, m_input_filename(std::move(input_filename))
		, m_current_source_manager()
		, m_source_cache()
		, m_current_source()
		, m_angled_inclusions()
	{ }

	virtual void ExecuteAction() override {
		auto &ci = getCompilerInstance();
		auto &pp = ci.getPreprocessor();
		auto &sm = ci.getSourceManager();
		m_current_source_manager = &sm;

		std::unordered_set<std::string> direct_inclusions;
		pp.addPPCallbacks(std::make_unique<InclusionHandler>(this));

		m_source_cache.clear();
		m_angled_inclusions.clear();
		const std::string input_filename{sm.getFilename(sm.getLocForStartOfFile(sm.getMainFileID()))};
		m_source_cache.emplace(input_filename, split_text(m_input_content));
		m_current_source = &m_source_cache[input_filename];

		pp.EnterMainSourceFile();
		std::ostringstream oss;
		int last_line = -1;
		for(;;){
			const auto before_current_source = m_current_source;
			clang::Token tok;
			pp.Lex(tok);
			if(tok.is(clang::tok::eof)){ break; }
			if(!m_current_source){ continue; }
			const auto loc = tok.getLocation();
			const int cur_line = sm.getPresumedLineNumber(loc) - 1;
			if(
				before_current_source != m_current_source ||
				cur_line != last_line)
			{
				oss << (*m_current_source)[cur_line] << std::endl;
			}
			last_line = cur_line;
		}
		std::ostringstream oss_stdcxx;
		for(const auto &s : m_angled_inclusions){
			oss_stdcxx << "#include <" << s << ">" << std::endl;
		}
		if(m_result_ptr){
			*m_result_ptr = oss_stdcxx.str() + oss.str();
		}
	}

private:
	std::vector<std::string> split_text(const std::string &text) const {
		std::istringstream iss(text);
		std::string line;
		std::vector<std::string> result;
		while(std::getline(iss, line)){
			result.emplace_back(std::move(line));
		}
		return result;
	}

	std::vector<std::string> load_text_file(const std::string &filename) const {
		std::ifstream ifs(filename.c_str());
		std::string line;
		std::vector<std::string> result;
		while(std::getline(ifs, line)){
			result.emplace_back(std::move(line));
		}
		return result;
	}

	void OnFileChanged(clang::SourceLocation loc){
		auto &sm = *m_current_source_manager;
		const auto path = sm.getFilename(loc).str();
		const auto it = m_source_cache.find(path);
		if(it != m_source_cache.end()){
			m_current_source = &it->second;
		}else{
			m_current_source = nullptr;
		}
	}

	void OnInclusionDirective(
		clang::SourceLocation hash_loc,
		clang::StringRef filename,
		bool is_angled,
		const clang::FileEntry *file)
	{
		const std::string path(file->getName());
		auto &sm = *m_current_source_manager;
		const auto from = sm.getFilename(hash_loc).str();
		if(m_source_cache.find(from) != m_source_cache.end()){
			//if(is_angled){
			//	m_angled_inclusions.insert(filename.str());
			//}else if(m_source_cache.find(path) == m_source_cache.end()){
				m_source_cache.emplace(path, load_text_file(path));
			//}
		}
	}

};

class InclusionUnrollingActionFactory
	: public clang::tooling::FrontendActionFactory
{

private:
	std::shared_ptr<std::string> m_result_ptr;
	std::string m_input_content;
	std::string m_input_filename;

public:
	InclusionUnrollingActionFactory(
		std::shared_ptr<std::string> result_ptr,
		std::string input_content,
		std::string input_filename)
		: m_result_ptr(std::move(result_ptr))
		, m_input_content(std::move(input_content))
		, m_input_filename(std::move(input_filename))
	{ }

	virtual std::unique_ptr<clang::FrontendAction> create() override {
		return std::make_unique<InclusionUnrollingAction>(
			m_result_ptr, m_input_content, m_input_filename);
	}

};


std::string unroll_inclusion(
	const std::string &input_source,
	const std::string &input_filename,
	const std::vector<std::string> &clang_options)
{
	namespace tooling = clang::tooling;
	tooling::FixedCompilationDatabase compilations(".", clang_options);
	tooling::ClangTool tool(
		compilations, llvm::ArrayRef<std::string>(input_filename));
	tool.mapVirtualFile(input_filename, input_source);

	auto result_ptr = std::make_shared<std::string>();
	tool.appendArgumentsAdjuster(tooling::getClangSyntaxOnlyAdjuster());
	const auto result = tool.run(
		std::make_unique<InclusionUnrollingActionFactory>(
			result_ptr, input_source, input_filename).get());

	return *result_ptr;
}

