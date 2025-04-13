#include "MacroEngine.hpp"
#include <cstdlib>
#include <vector>
#include <iostream>
#include <sstream>

extern "C" {
	#include <sys/wait.h>
}

#include "MacroParser.hpp"
#include "DEBUG.hpp"

using namespace std;
using namespace pugi;


// ----------------------------------- [ Structures ] --------------------------------------- //


constexpr size_t STDIN_CHUNK_SIZE = 4096;


struct ShellCmd {
	const char* cmd = nullptr;
	const Expression::VariableMap* vars = nullptr;
	const vector<string_view>* env = nullptr;
	string* capture = nullptr;
	const filesystem::path* cwd = nullptr;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


// Called from child process
static void _setEnv(const Expression::VariableMap& vars, const vector<string_view>& env){
	string buff = {};
	
	for (string_view namev : env){
		
		auto p = vars.find(namev);
		if (p == vars.end()){
			continue;
		}
		
		buff.clear();
		const string& name = p->first;
		const Expression::Value& val = p->second;
		Expression::str(val, buff);
		
		setenv(name.c_str(), buff.c_str(), 1);
	}
	
}


static int _shell(const ShellCmd& cmd) noexcept {
	assert(cmd.cmd != nullptr);
	if (cmd.cmd == nullptr){
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
		if (cmd.cwd != nullptr){
			try {
				if (filesystem::is_directory(*cmd.cwd))
					filesystem::current_path(*cmd.cwd);
				else
					filesystem::current_path(cmd.cwd->parent_path());
			} catch (const exception&) {
				exit(102);
			}
		}
		
		if (cmd.env != nullptr && cmd.vars != nullptr){
			_setEnv(*cmd.vars, *cmd.env);
		}
		
		execlp("bash", "bash", "-c", cmd.cmd, NULL);
		exit(1);
	}
	
	// Parent
	else if (pid > 0){
		if (cmd.capture != nullptr){
			close(fd[1]);
			
			// Fill array of buffers of STDIN_CHUNK_SIZE that are combined into final string after EOF.
			size_t totalSize = 0;
			vector<vector<char>> buffs = {};
			buffs.reserve(4);
			
			while (true){
				vector<char>& buff = buffs.emplace_back();
				buff.resize(STDIN_CHUNK_SIZE);
				
				size_t space = buff.size();
				size_t offset = 0;
				
				// Fill one buffer
				while (space > 0){
					
					const ssize_t n = read(fd[0], buff.data() + offset, space);
					if (n <= 0){
						buff.resize(offset);
						goto __closed;
					}
					
					space -= n;
					offset += n;
					totalSize += n;
				}
				
			}
			
			__closed:
			close(fd[0]);
			
			// Build result from intermediate STDIN_CHUNK_SIZE buffers.
			string& out = *cmd.capture;
			out.reserve(out.size() + totalSize);
			for (const vector<char>& buff : buffs){
				out.append(buff.begin(), buff.end());
			}
			
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
	
	return -1;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void _extractVars(const char* csv, vector<string_view>& vars){
	const char* beg = nullptr;
	
	while (*csv != 0){
		
		if (isspace(*csv) || *csv == ','){
			if (beg != nullptr){
				vars.emplace_back(string_view(beg, csv));
				beg = nullptr;
			}
		} else if (beg == nullptr){
			beg = csv;
		}
		
		csv++;
	}
	
	if (beg != nullptr){
		vars.emplace_back(string_view(beg, csv));
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::shell(const xml_node op, xml_node dst){
	vector<string_view> vars = {};
	
	enum class Capture {
		VOID,
		PCDATA,
		HTML,
		VAR
	} capture = Capture::PCDATA;
	
	string_view captureVar = {};
	
	
	for (const xml_attribute attr : op.attributes()){
		string_view name = attr.name();
		
		if (name == "IF"){
			optbool val = evalCond(attr.value());
			
			if (val.empty()){
				WARNING_L1("SHELL: Invalid expression in macro attribute [IF=\"%s\". Defaulting to false.", attr.value());
				return true;
			} else if (val == false){
				return true;
			}
			
		}
		else if (name == "VARS"){
			_extractVars(attr.value(), vars);
		}
		else if (name == "STDOUT"){
			string_view val = attr.value();
			
			if (val == ""){
				capture = Capture::VOID;
			} else if (val == "HTML"){
				capture = Capture::HTML;
			} else if (val == "TEXT"){
				capture = Capture::PCDATA;
			} else {
				capture = Capture::VAR;
				captureVar = val;
			}
			
		}
		else {
			WARNING_L1("SHELL: Ignored unknown macro attribute [%s=\"%s\"].", attr.name(), attr.value());
		}
		
	}
	
	
	int status;
	string result;
	
	ShellCmd cmd = {
		.vars = &this->variables,
		.env = &vars,
		.capture = (capture != Capture::VOID) ? &result : nullptr
	};
	
	// Only one text child
	xml_node content = op.first_child();
	if (content.next_sibling().empty() && !content.text().empty()){
		cmd.cmd = content.value();
		status = _shell(cmd);
	}
	
	// Build command from child nodes
	else {
		stringstream ss = {};
		
		for (const xml_node child : op){
			child.print(ss, "", format_raw);
		}
		
		string cmdbuff = move(ss).str();
		cmd.cmd = cmdbuff.c_str();
		status = _shell(cmd);
	}
	
	if (status != 0){
		ERROR_L1("SHELL: Exited with status (" ANSI_RED "%d" ANSI_RESET ").", status);
	}
	
	// Apply result
	switch (capture){
		case Capture::VOID: {
			return true;
		}
		
		case Capture::PCDATA: {
			xml_text txt = dst.append_child(xml_node_type::node_pcdata).text();
			txt.set(result.c_str(), result.length());
			return true;
		}
		
		case Capture::HTML: {
			return execBuff(string_view(result), dst);
		}
		
		case Capture::VAR: {
			if (!captureVar.empty())
				variables[string(captureVar)] = move(result);
			return true;
		}
		
	}
	
	
	return true;
}


// ------------------------------------------------------------------------------------------ //