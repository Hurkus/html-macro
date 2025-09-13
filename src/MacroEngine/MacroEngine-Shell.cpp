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


constexpr size_t STDIN_CHUNK_SIZE = 1024;


struct str_chunks {
	vector<vector<char>> chunks;
	size_t total = 0;
};


struct ShellCmd {
	string_view cmd;
	const VariableMap* vars = nullptr;
	const vector<string_view>* env = nullptr;
	str_chunks* capture = nullptr;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void _slurp(int fd, str_chunks& out_chunks){
	out_chunks.chunks.reserve(4);
	
	while (true){
		vector<char>& buff = out_chunks.chunks.emplace_back();
		buff.resize(out_chunks.chunks.size() * STDIN_CHUNK_SIZE);	// Each larger than previous.
		
		size_t space = buff.size();
		size_t offset = 0;
		
		// Fill one buffer
		while (space > 0){
			const ssize_t n = read(fd, buff.data() + offset, space);
			if (n <= 0){
				buff.resize(offset);
				return;
			}
			
			space -= size_t(n);
			offset += size_t(n);
			out_chunks.total += size_t(n);
		}
		
	}
	
}


static size_t concat(const vector<vector<char>>& chunks, char* buff, size_t buff_len){
	size_t n = 0;
	
	for (const vector<char>& chunk : chunks){
		if (n >= buff_len) break;
		size_t space = buff_len - n;
		size_t count = min(chunk.size(), space);
		memcpy(buff + n, chunk.data(), count);
		n += count;
	}
	
	return n;
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
static void _setEnv(const VariableMap& vars, const vector<string_view>& env){
	string buff = {};
	
	for (string_view name : env){
		const auto* keyval = vars.find(name);
		if (keyval == nullptr){
			continue;
		}
		
		buff.clear();
		toStr(keyval->value, buff);
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


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::shell(const Node& op, Node& dst){
	// Verify command text exists
	Node* content = op.child;
	if (content == nullptr){
		HERE(warn_text_missing(op));
		return;
	} else if (content->type != NodeType::TEXT){
		HERE(warn_text_missing(op));
		return;
	}
	
	string_view cmdtxt = trim_whitespace(content->value());
	if (cmdtxt.empty()){
		HERE(warn_text_missing(op));
		return;
	}
	
	enum class Capture {
		VOID, TEXT, VAR
	} capture = Capture::TEXT;
	
	vector<string_view> vars = {};
	string_view captureVar = {};
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "VARS"){
			_extractVars(attr->value(), vars);
			continue;
		} else if (name == "STDOUT"){
			string_view val = attr->value();
			
			if (val == "" || val == "VOID"){
				capture = Capture::VOID;
			} else if (val == "TEXT"){
				capture = Capture::TEXT;
			} else {
				capture = Capture::VAR;
				captureVar = val;
			}
			
			continue;
		}
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		HERE(warn_ignored_attribute(op, *attr));
	}
	
	// Run command
	str_chunks result;
	ShellCmd cmd = {
		.cmd = cmdtxt,
		.vars = &MacroEngine::variables,
		.env = &vars,
		.capture = (capture != Capture::VOID) ? &result : nullptr
	};
	
	int status = _shell(cmd);
	if (status != 0){
		HERE(warn_shell_exit(op, status));
		return;
	}
	
	// Apply result
	switch (capture){
		case Capture::VOID: {
			break;
		}
		
		case Capture::TEXT: {
			if (result.total > 0){
				Node& txt = dst.appendChild(NodeType::TEXT);
				
				char* s = html::newStr(result.total + 1);
				size_t n = concat(result.chunks, s, result.total);
				s[n] = 0;
				
				// Trim last newline
				if (n > 0 && s[n-1] == '\n'){
					s[--n] = 0;
				}
				
				txt.value(s, n);
			}
		} break;
		
		case Capture::VAR: {
			if (result.total <= 0){
				variables.insert(captureVar, in_place_type<string>);
			} else {
				string& s = variables[captureVar].emplace<string>();
				s.resize(result.total);
				size_t n = concat(result.chunks, s.data(), s.size());
				s.resize(n);
				
				// Trim last newline
				if (s.ends_with('\n')){
					s.pop_back();
				}
				
			}
		} break;
		
	}
	
}


// ------------------------------------------------------------------------------------------ //