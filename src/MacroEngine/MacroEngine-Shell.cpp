#include "MacroEngine.hpp"
#include <thread>
#include <sys/wait.h>

#include "fd.hpp"
#include "Paths.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;


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


static void slurp(int fd, str_chunks& out_chunks){
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


constexpr string_view trim(string_view s) noexcept {
	const char* beg = s.begin();
	const char* end = s.end();
	
	while (beg != end && isspace(beg[+0])) beg++;
	while (end != beg && isspace(end[-1])) end--;
	
	return string_view(beg, end);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


// Called from child process
static void _setEnv(const VariableMap& vars, const vector<string_view>& env){
	char buff[96];
	
	for (string_view name : env){
		const auto* keyval = vars.find(name);
		if (keyval == nullptr){
			continue;
		}
		
		const Value& val = keyval->value;
		
		switch (val.type){
			case Value::Type::LONG: {
				if (snprintf(buff, sizeof(buff), "%ld", val.data.l) >= 0)
					setenv(keyval->key, buff, 1);
				else
					assert(false);
			} break;
			
			case Value::Type::DOUBLE: {
				if (snprintf(buff, sizeof(buff), "%lf", val.data.d) >= 0)
					setenv(keyval->key, buff, 1);
				else
					assert(false);
			} break;
			
			case Value::Type::STRING: {
				assert(val.data.s[val.data_len] == 0);
				setenv(keyval->key, val.data.s, 1);
			} break;
		}
		
	}
	
}


static int _shell(const ShellCmd& cmd){
	assert(Paths::cwd != nullptr);
	assert(fs::is_dir(*Paths::cwd));
	
	if (cmd.cmd.empty()){
		assert(!cmd.cmd.empty());
		return 0;
	}
	
	fs::FileDesc in0;
	fs::FileDesc in1;
	fs::FileDesc out0;
	fs::FileDesc out1;
	if (!fs::pipe(in0, in1)){
		return 100;
	} else if (cmd.capture != nullptr && !fs::pipe(out0, out1)){
		return 100;
	}
	
	const pid_t pid = fork();
	
	// Child
	if (pid == 0){
		if (dup2(in0, 0) < 0){
			exit(101);
		} else if (cmd.capture == nullptr){
			close(1);
			close(2);
		} else if (dup2(out1, 1) < 0){
			exit(101);
		}
		
		in0.close();
		in1.close();
		out0.close();
		out1.close();
		
		// Set current directory
		if (!fs::cwd(*Paths::cwd)){
			exit(102);
		}
		
		// Set environment variables
		if (cmd.env != nullptr && cmd.vars != nullptr){
			_setEnv(*cmd.vars, *cmd.env);
		}
		
		// Input comes on stdin
		execlp("bash", "bash", NULL);
		exit(1);
	}
	
	// Parent
	else if (pid > 0){
		in0.close();
		out1.close();
		
		// Thread capturing output while main writes
		thread reader;
		
		if (cmd.capture != nullptr){
			reader = thread([&](){
				slurp(out0, *cmd.capture);
			});
		}
		
		// Write command to stdin of bash
		{
			const char* p = cmd.cmd.data();
			const size_t total = cmd.cmd.length();
			size_t wcount = 0;
			
			while (wcount < total){
				ssize_t n = write(in1, p + wcount, total - wcount);
				if (n < 0)
					break;
				wcount += size_t(n);
			}
			
			in1.close();
		}
		
		if (out0.isOpen()){
			reader.join();
			out0.close();
		}
		
		// Exit child
		int err = 0;
		waitpid(pid, &err, 0);
		return WEXITSTATUS(err);
	}
	
	// Error
	else {
		assert(false);
		return 105;
	}
	
	return 255;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


// Comma separated list of names
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
	assert(macro != nullptr);
	assert(macro->html != nullptr);
	assert(variables != nullptr);
	
	// Verify command text exists
	Node* content = op.child;
	if (content == nullptr || content->type != NodeType::TEXT){
		HERE(error_missing_text(*macro, op));
		return;
	} else if (content->next != nullptr){
		HERE(warn_ignored_child(*macro, op, content->next));
	}
	
	string_view cmdtxt = trim(content->value());
	if (cmdtxt.empty()){
		HERE(error_missing_text(*macro, op));
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
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		HERE(warn_ignored_attr(*macro, *attr));
	}
	
	// Run command
	str_chunks result;
	ShellCmd cmd = {
		.cmd = cmdtxt,
		.vars = variables.get(),
		.env = &vars,
		.capture = (capture != Capture::VOID) ? &result : nullptr
	};
	
	try {
		int status = _shell(cmd);
		if (status != 0){
			HERE(warn_shell_exit(*macro, op, status));
			return;
		}
	} catch (...){
		HERE(warn_shell_exit(*macro, op, -1));
		return;
	}
	
	// Apply result
	switch (capture){
		case Capture::VOID: {
			break;
		}
		
		case Capture::TEXT: {
			if (result.total > 0){
				char* s = newStr(result.total + 1);
				size_t n = concat(result.chunks, s, result.total);
				s[n] = 0;
				
				// Trim last newline
				if (n > 0 && s[n-1] == '\n'){
					s[--n] = 0;
				}
				
				Node& txt = *dst.appendChild(newNode(NodeType::TEXT));
				txt.options = op.options & (NodeOptions::SPACE_BEFORE | NodeOptions::SPACE_AFTER);
				txt.value_len = n;
				txt.value_p = s;
			}
		} break;
		
		case Capture::VAR: {
			size_t len = min(result.total, size_t(UINT32_MAX));
			if (len <= 0){
				variables->insert(captureVar, ""sv);
				break;
			}
			
			char* s = new char[len + 1];
			size_t n = concat(result.chunks, s, len);
			s[n] = 0;
			
			// Trim last newline
			if (n > 0 && s[n-1] == '\n'){
				s[--n] = 0;
			}
			
			assert(n <= UINT32_MAX);
			variables->insert(captureVar, s, uint32_t(n));
		} break;
		
	}
	
}


// ------------------------------------------------------------------------------------------ //