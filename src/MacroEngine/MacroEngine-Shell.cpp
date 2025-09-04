#include "MacroEngine.hpp"
#include <cstdlib>
#include <vector>
#include <iostream>
#include <sstream>

extern "C" {
	#include <sys/wait.h>
}

#include "Paths.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Structures ] --------------------------------------- //


constexpr size_t STDIN_CHUNK_SIZE = 4096;


struct ShellCmd {
	string_view cmd;
	const Expression::VariableMap* vars = nullptr;
	const vector<string_view>* env = nullptr;
	string* capture = nullptr;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


static size_t _slurp(int fd, string& out){
	// Array chunks that are concatenated into final string after EOF.
	size_t totalSize = 0;
	vector<vector<char>> buffs = {};
	buffs.reserve(4);
	
	while (true){
		vector<char>& buff = buffs.emplace_back();
		buff.resize(buffs.size() * STDIN_CHUNK_SIZE);	// Each larger than previous.
		
		size_t space = buff.size();
		size_t offset = 0;
		
		// Fill one buffer
		while (space > 0){
			const ssize_t n = read(fd, buff.data() + offset, space);
			if (n <= 0){
				buff.resize(offset);
				goto eof;
			}
			
			space -= size_t(n);
			offset += size_t(n);
			totalSize += size_t(n);
		}
		
	} eof:
	
	// Concatenate buffers
	out.reserve(out.length() + totalSize);
	for (const vector<char>& buff : buffs){
		out.append(buff.begin(), buff.end());
	}
	
	return totalSize;
}


static string_view trim_whitespace(string_view s){
	const char* beg = s.begin();
	const char* end = beg + s.length();
	
	while (beg != end && isspace(beg[+0])) beg++;
	while (end != beg && isspace(end[-1])) end--;
	
	return string_view(beg, end);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


// Called from child process
static void _setEnv(const Expression::VariableMap& vars, const vector<string_view>& env){
	string buff = {};
	
	for (string_view name : env){
		const auto* keyval = vars.find(name);
		if (keyval == nullptr){
			continue;
		}
		
		buff.clear();
		Expression::str(keyval->value, buff);
		setenv(keyval->key, buff.c_str(), 1);
	}
	
}


static int _shell(const ShellCmd& cmd) noexcept {
	assert(MacroEngine::cwd != nullptr);
	assert(filesystem::is_directory(*MacroEngine::cwd));
	assert(!cmd.cmd.empty());
	
	if (cmd.cmd.empty()){
		return 0;
	}
	
	int fd[2];
	if (cmd.capture != nullptr){
		if (pipe(fd) != 0)
			return 100;
	}
	
	const int pid = fork();
	
	// Child
	if (pid == 0){
		
		if (cmd.capture != nullptr){
			if (dup2(fd[1], 1) == -1){
				exit(101);
			}
			
			close(fd[0]);
			close(fd[1]);
		}
		
		// Set current directory
		try {
			filesystem::current_path(*MacroEngine::cwd);
		} catch (const exception&) {
			exit(102);
		}
		
		if (cmd.env != nullptr && cmd.vars != nullptr){
			_setEnv(*cmd.vars, *cmd.env);
		}
		
		string cmd_s = string(trim_whitespace(cmd.cmd));
		execlp("bash", "bash", "-c", cmd_s.c_str(), NULL);
		exit(1);
	}
	
	// Parent
	else if (pid > 0){
		if (cmd.capture != nullptr){
			close(fd[1]);
			_slurp(fd[0], *cmd.capture);
			close(fd[0]);
		}
		
		// Exit child
		int err = 0;
		waitpid(pid, &err, 0);
		return WEXITSTATUS(err);
	}
	
	else {
		close(fd[0]);
		close(fd[1]);
		assert(false);
		return 105;
	}
	
	return 255;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void _extractVars(string_view csv, vector<string_view>& vars){
	const char* p = csv.data();
	const char* end = p + csv.length();
	const char* beg = nullptr;
	
	while (p != end){
		
		if (isspace(*p) || *p == ','){
			if (beg != nullptr){
				vars.emplace_back(string_view(beg, p));
				beg = nullptr;
			}
		} else if (beg == nullptr){
			beg = p;
		}
		
		p++;
	}
	
	if (beg != nullptr){
		vars.emplace_back(string_view(beg, end));
	}
	
}


static void _execBuff(string&& buff, Node& dst){
	// MEMORY LEAK, buffer of `m` should not be freed untill the main document releases it's data.
	// unique_ptr<Macro> m = Macro::loadBuffer(make_shared<string>(move(buff)));
	// if (m != nullptr)
	// 	MacroEngine::exec(*m, dst);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::shell(const Node& op, Node& dst){
	enum class Capture {
		VOID,
		TEXT,
		HTML,
		VAR
	} capture = Capture::TEXT;
	
	vector<string_view> vars = {};
	string_view captureVar = {};
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "IF"){
			if (!eval_attr_if(op, *attr))
				return;
		} else if (name == "VARS"){
			_extractVars(attr->value(), vars);
		} else if (name == "STDOUT"){
			string_view val = attr->value();
			
			if (val == "" || val == "VOID"){
				capture = Capture::VOID;
			} else if (val == "HTML"){
				capture = Capture::HTML;
			} else if (val == "TEXT"){
				capture = Capture::TEXT;
			} else {
				capture = Capture::VAR;
				captureVar = val;
			}
			
		} else {
			warn_ignored_attribute(op, *attr);
		}
		
	}
	
	
	int status;
	string result;
	
	ShellCmd cmd = {
		.vars = &MacroEngine::variables,
		.env = &vars,
		.capture = (capture != Capture::VOID) ? &result : nullptr
	};
	
	// Read command from child node.
	Node* content = op.child;
	if (content == nullptr){
		::warn(op, "Unused shell command node.");
		return;
	} else if (content->type != NodeType::TEXT){
		::error(op, "Plaintext expected for shell command.");
		return;
	}
	
	cmd.cmd = content->value();
	status = _shell(cmd);
	if (status != 0){
		::warn_shell_exit(op, status);
		return;
	}
	
	// Trim last newline
	if (result.ends_with('\n')){
		result.pop_back();
	}
	
	// Apply result
	switch (capture){
		case Capture::VOID: {
			break;
		}
		
		case Capture::TEXT: {
			Node& txt = dst.appendChild(NodeType::TEXT);
			txt.value(html::newStr(result), result.length());
			break;
		}
		
		case Capture::HTML: {
			_execBuff(move(result), dst);
			::error(op, "HTML stdout not yet implemented.");
			break;
		}
		
		case Capture::VAR: {
			variables[captureVar] = move(result);
			break;
		}
		
	}
	
}


// ------------------------------------------------------------------------------------------ //