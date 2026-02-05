#include "test.hpp"
#include <cassert>
#include <fstream>
#include <future>
#include <fcntl.h>
#include <sys/wait.h>


using namespace std;

#define RB(s)	ANSI_BOLD ANSI_RED s ANSI_RESET
#define GB(s)	ANSI_BOLD ANSI_GREEN s ANSI_RESET


// ----------------------------------- [ Variables ] ---------------------------------------- //


bool stderr_isTTY = true;
bool stdout_isTTY = true;

constexpr const char* ENV_HTML_MACRO = "PROG";
filepath html_macro = "./bin/html-macro";

filepath TmpFile::dir;

TestList* tests;


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool slurp(int fd, string& output){
	char buff[4096];
	
	while (true){
		const ssize_t n = read(fd, buff, sizeof(buff));
		if (n < 0)
			return false;
		else if (n == 0)
			return true;
		else
			output.append(buff, size_t(n));
	}
	
	return false;
}


bool slurp(const filepath& path, string& buff){
	fs::FileDesc fd = open(path.c_str(), O_RDONLY);
	if (fd < 0)
		return false;
	return slurp(fd, buff);
}


string slurp(const filepath& path){
	string buff;
	if (!slurp(path, buff))
		buff.clear();
	return buff;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


TmpFile::TmpFile(string_view name, string_view content){
	if (dir.empty()){
		dir = "/tmp/html-macro-test";
		filesystem::create_directory(dir);
	}
	
	path = dir / name;
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
		if (!TmpFile::dir.empty())
			filesystem::remove_all(TmpFile::dir);
	}
} _TmpFile_static;


// ----------------------------------- [ Functions ] ---------------------------------------- //


int exe(const vector<string>& args, string& out, string& err){
	fs::FileDesc out0;
	fs::FileDesc out1;
	fs::FileDesc err0;
	fs::FileDesc err1;
	
	if (!fs::pipe(out0, out1) || !fs::pipe(err0, err1)){
		ERROR("Internal error when creating a pipe.");
		return 100;
	}
	
	const pid_t pid = fork();
	
	// Child
	if (pid == 0){
		dup2(out1, 1);
		dup2(err1, 2);
		out0.close();
		out1.close();
		err0.close();
		err1.close();
		
		// Build argument array
		vector<const char*> argv;
		argv.emplace_back(html_macro.c_str());
		for (const string& arg : args)
			argv.emplace_back(arg.c_str());
		argv.emplace_back(nullptr);
		
		execvp(html_macro.c_str(), (char**)argv.data());
		exit(1);
	}
	
	// Parent
	else if (pid > 0){
		out1.close();
		err1.close();
		
		future reader1 = async(launch::async, [&](){
			slurp(out0, out);
		});
		future reader2 = async(launch::async, [&](){
			slurp(err0, err);
		});
		
		bool ok = true;
		ok &= (reader1.wait_for(1s) == future_status::ready);
		ok &= (reader2.wait_for(1s) == future_status::ready);
		out0.close();
		out1.close();
			
		if (!ok){
			fprintf(stderr, RB("error: ") "Timeout.\n");
			kill(pid, SIGKILL);
			reader1.wait_for(1s);
			reader2.wait_for(1s);
		}
		
		int status;
		waitpid(pid, &status, 0);
		return WEXITSTATUS(status);
	}
	
	return 100;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


struct shell_exception {
	int status = 1;
	int expected = 0;
	std::string err;
};

struct output_exception {
	std::string expected;
	std::string recieved;
};


bool run(const vector<string>& args, string_view out, string_view err, int status){
	string out_buff;
	string err_buff;
	
	int _status = exe(args, out_buff, err_buff);
	if (_status != status)
		throw shell_exception{_status, status, move(err_buff)};
	else if (err_buff != err)
		throw output_exception{string(err), move(err_buff)};
	else if (out_buff != out)
		throw output_exception{string(out), move(out_buff)};
	
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
	
	for (const TestList* test = tests ; test != nullptr ; test = test->next){
		if (test->func == nullptr){
			continue;
		}
		
		// if (string_view(test->name) != "doc_macro_SHELL"){
		// 	continue;
		// }
		
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
			printf("Program exited with status " PURPLE("%d") ", expected " PURPLE("%d") ".\n", e.status, e.expected);
			// printf("%s", e.err.c_str());
		}
		catch (const output_exception& e){
			printf(RB("FAIL: ") "%s ~ %s:%ld\n", test->name, test->file, test->line);
			fmt_expected(e.expected, e.recieved);
			printf(ANSI_PURPLE "FROM: %s ~ %s:%ld" ANSI_RESET "\n", test->name, test->file, test->line);
		}
		
		total++;
	}
	
	printf("--------------------------------\n");
	if (passed == total)
		printf("Passed " GREEN("%i") "/" GREEN("%i") " ~ " GREEN("%.0f%%") " of tests.\n", passed, total, 100.0f*passed/total);
	else
		printf("Passed " YELLOW("%i") "/" GREEN("%i") " ~ " YELLOW("%.0f%%") " of tests.\n", passed, total, 100.0f*passed/total);
	printf("--------------------------------\n");
	
	return passed == total;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	const char* prog_cpath = getenv(ENV_HTML_MACRO);
	if (prog_cpath != nullptr){
		html_macro = prog_cpath;
	}
	
	if (!fs::is_file(html_macro)){
		fprintf(stderr, RB("error: ") "Program " PURPLE("`%s`") " does not exist.\n", html_macro.c_str());
		return 1;
	}
	
	if (!run()){
		return 1;
	}
	
	return 0;
}


// ------------------------------------------------------------------------------------------ //