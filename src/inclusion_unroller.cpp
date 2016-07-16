#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <clang/Basic/FileManager.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
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
			const clang::Module *) override
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
		m_source_cache.emplace(m_input_filename, split_text(m_input_content));
		m_current_source = &m_source_cache[m_input_filename];

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
		if(is_angled){
			auto &sm = *m_current_source_manager;
			const auto from = sm.getFilename(hash_loc).str();
			if(m_source_cache.find(from) != m_source_cache.end()){
				m_angled_inclusions.insert(filename.str());
			}
		}else{
			if(m_source_cache.find(path) == m_source_cache.end()){
				m_source_cache.emplace(path, load_text_file(path));
			}
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

	virtual clang::FrontendAction *create() override {
		return new InclusionUnrollingAction(
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

