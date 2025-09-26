#include "test.hpp"
#include <cassert>
#include <vector>
#include <fstream>

extern "C" {
	#include <poll.h>
	#include <fcntl.h>
	#include <sys/wait.h>
}

using namespace std;


#define R(s)	ANSI_RED s ANSI_RESET
#define B(s)	ANSI_BOLD s ANSI_RESET
#define RB(s)	ANSI_BOLD ANSI_RED s ANSI_RESET
#define GB(s)	ANSI_BOLD ANSI_GREEN s ANSI_RESET


// ----------------------------------- [ Variables ] ---------------------------------------- //


constexpr const char* ENV_HTML_MACRO = "PROG";
filepath html_macro = "./bin/html-macro";

filepath TmpFile::dir;

TestList* tests;


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool slurp(int fd, string& buff){
	vector<char> chunk = vector<char>(4096);
	
	while (true){
		const ssize_t n = read(fd, chunk.data(), chunk.size());
		if (n < 0)
			return false;
		else if (n == 0)
			return true;
		else
			buff.append(chunk.data(), size_t(n));
	}
	
	return false;
}


bool slurp(const filepath& path, string& buff){
	const int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0){
		return false;
	}
	
	bool err = slurp(fd, buff);
	close(fd);
	return err;
}


string slurp(const filepath& path){
	string buff;
	if (!slurp(path, buff))
		buff.clear();
	return buff;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


TmpFile::TmpFile(string_view name, const string_view& content){
	if (dir.empty()){
		dir = "/tmp/html-macro-test";
		filesystem::create_directory(dir);
	}
	
	path = dir / (string(name) + ".html");
	ofstream out = ofstream(path);
	out.write(content.data(), content.length());
	out.close();
}


TmpFile::~TmpFile(){
	if (!path.empty()){
		error_code _err;
		filesystem::remove(path, _err);
	}
}


struct _TmpFile_static_t {
	~_TmpFile_static_t(){
		if (!TmpFile::dir.empty()){
			filesystem::remove_all(TmpFile::dir);
		}
	}
} _TmpFile_static;


// ----------------------------------- [ Functions ] ---------------------------------------- //


int exe(const vector<string>& args, string& out, string& err){
	int p_stdout[2];
	int p_stderr[2];
	
	if (pipe(p_stdout) != 0){
		fprintf(stderr, RB("error: ") "Internal error when creating a pipe.\n");
		return 1;
	} else if (pipe(p_stderr) != 0){
		close(p_stdout[0]);
		close(p_stdout[1]);
		fprintf(stderr, RB("error: ") "Internal error when creating a pipe.\n");
		return 1;
	}
	
	const pid_t pid = fork();
	
	// Child
	if (pid == 0){
		close(0);
		
		dup2(p_stdout[1], 1);
		close(p_stdout[0]);
		close(p_stdout[1]);
		
		dup2(p_stderr[1], 2);
		close(p_stderr[0]);
		close(p_stderr[1]);
		
		// Build argument array
		vector<const char*> _args;
		_args.emplace_back(html_macro.c_str());
		for (const string& arg : args)
			_args.emplace_back(arg.c_str());
		_args.emplace_back(nullptr);
		
		execvp(html_macro.c_str(), (char**)_args.data());
		exit(1);
	}
	
	// Parent
	else if (pid > 0){
		close(p_stdout[1]);
		close(p_stderr[1]);
		
		vector<pollfd> info = {
			pollfd {
				.fd = p_stdout[0],
				.events = POLLIN
			},
			pollfd {
				.fd = p_stderr[0],
				.events = POLLIN
			}
		};
		
		fcntl(info[0].fd, F_SETFL, fcntl(info[0].fd, F_GETFL, 0) | O_NONBLOCK);
		fcntl(info[1].fd, F_SETFL, fcntl(info[1].fd, F_GETFL, 0) | O_NONBLOCK);
		
		while (!info.empty()){
			if (poll(info.data(), info.size(), 1000) <= 0){
				break;
			}
			
			for (int i = int(info.size()) - 1 ; i >= 0 ; i--){
				
				// Read input
				if (bool(info[i].revents & POLLIN)){
					if (info[i].fd == p_stdout[0])
						slurp(info[i].fd, out);
					else if (info[i].fd == p_stderr[0])
						slurp(info[i].fd, err);
				}
				
				// Close
				if (bool(info[i].revents & (POLLHUP | POLLERR))){
					info.erase(info.begin() + i);
				}
				
			}
			
		}
			
		close(p_stdout[0]);
		close(p_stderr[0]);
		
		// Get return status of child.
		int err = 0;
		for (int ms = 0 ; ms < 500 ; ms++){
			
			if (waitpid(pid, &err, WNOHANG) == 0){
				usleep(1000);
				continue;
			}
			
			// ok
			return WEXITSTATUS(err);
		}
		
		// Timeout
		fprintf(stderr, RB("error: ") "Timeout.\n");
		kill(pid, SIGKILL);
		waitpid(pid, &err, 0);
		return 1;
	}
	
	// Error
	close(p_stdout[0]);
	close(p_stdout[1]);
	close(p_stderr[0]);
	close(p_stderr[1]);
	return 1;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


struct shell_exception {
	int status = 1;
	std::string err;
};

struct output_exception {
	std::string expected;
	std::string recieved;
};


bool run(const vector<string>& args, const string& out, const string& err, int status){
	string out_buff;
	string err_buff;
	
	int _status = exe(args, out_buff, err_buff);
	if (_status != status)
		throw shell_exception{_status, move(err_buff)};
	else if (out_buff != out)
		throw output_exception{out, move(out_buff)};
	else if (err_buff != err)
		throw output_exception{err, move(err_buff)};
	
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void fmt_expected(const string& expected, const string& recieved){
	printf(RB(ANSI_UNDERLINE "Expected:") "\n");
	
	int ln = 1;
	const char* p1 = expected.begin().base();
	const char* e1 = expected.end().base();
	
	while (p1 != e1){
		const char* b1 = p1;
		while (p1 != e1 && *p1 != '\n'){
			p1++;
		}
		
		printf("%-3d|%.*s\n", ln, int(p1 - b1), b1);
		
		// \n
		if (p1 != e1)
			p1++;
		
		ln++;
	}
	
	printf(RB(ANSI_UNDERLINE "Recieved:") "\n");
	
	ln = 1;
	p1 = expected.begin().base();
	e1 = expected.end().base();
	const char* p2 = recieved.begin().base();
	const char* e2 = recieved.end().base();
	
	while (p2 != e2){
		const char* b1 = p1;
		const char* b2 = p2;
		const char* m = nullptr;
		
		// Read line and compare
		while (p2 != e2){
			if (p1 == e1 || *p1 != *p2){
				m = p2;
				break;
			} else if (*p1 == '\n'){
				break;
			} else {
				p1++;
				p2++;
			}
		}
		
		// Finish reading lines
		while (p1 != e1 && *p1 != '\n') p1++;
		while (p2 != e2 && *p2 != '\n') p2++;
		
		if (m == nullptr){
			printf(ANSI_GREEN "%-3d|%.*s" ANSI_RESET "\n", ln, int(p2 - b2), b2);
		} else {
			printf(ANSI_YELLOW "%-3d|%.*s" ANSI_RED "%.*s" ANSI_RESET "\n", ln, int(m - b2), b2, int(p2 - m), m);
		}
		
		// \n
		if (p1 != e1)
			p1++;
		if (p2 != e2)
			p2++;
		
		ln++;
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool run(){
	printf("--------------------------------\n");
	int total = 0;
	int passed = 0;
	
	const TestList* test = tests;
	while (test != nullptr){
		if (test->func == nullptr){
			test = test->next;
			continue;
		}
		
		try {
			if (test->func()){
				printf(GB("PASS: ") "%s ~ %s:%ld\n", test->name, test->file, test->line);
				passed++;
			} else {
				printf(RB("FAIL: ") "%s ~ %s:%ld\n", test->name, test->file, test->line);
			}
		}
		catch (const shell_exception& e){
			printf(RB("FAIL: ") "%s ~ %s:%ld\n", test->name, test->file, test->line);
			printf("Program exited with status (%d).\n", e.status);
			printf("%s", e.err.c_str());
		}
		catch (const output_exception& e){
			printf(RB("FAIL: ") "%s ~ %s:%ld\n", test->name, test->file, test->line);
			fmt_expected(e.expected, e.recieved);
			printf(ANSI_PURPLE "FROM: %s ~ %s:%ld" ANSI_RESET "\n", test->name, test->file, test->line);
		}
		
		total++;
		test = test->next;
	}
	
	printf("--------------------------------\n");
	if (passed == total){
		printf("Passed " ANSI_GREEN "%ld" ANSI_RESET "/" ANSI_GREEN "%ld" ANSI_RESET " tests.\n", passed, total);
	} else {
		printf("Passed " ANSI_YELLOW "%ld" ANSI_RESET "/" ANSI_GREEN "%ld" ANSI_RESET " tests.\n", passed, total);
	}
	printf("--------------------------------\n");
	
	return passed == total;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	const char* prog_cpath = getenv(ENV_HTML_MACRO);
	if (prog_cpath != nullptr){
		html_macro = prog_cpath;
	}
	
	if (!filesystem::exists(html_macro)){
		fprintf(stderr, RB("error: ") "Program '%s' does not exist.\n", html_macro.c_str());
		return 1;
	}
	
	if (!run()){
		return 1;
	}
	
	return 0;
}


// ------------------------------------------------------------------------------------------ //