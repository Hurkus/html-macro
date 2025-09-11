#include <cassert>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <tuple>
#include <filesystem>

extern "C" {
	#include <poll.h>
	#include <fcntl.h>
	#include <sys/wait.h>
}

#include "ANSI.h"
#include "Debug.hpp"

using namespace std;


// ----------------------------------- [ Constants ] ---------------------------------------- //


constexpr const char* ENV_HTML_MACRO = "PROG";
constexpr string_view INPUT_SUFFIX = ".in.html";
constexpr string_view OUTPUT_SUFFIX = ".out.html";


constexpr array testFiles = {
	tuple("test/test-1.in.html", "test/test-1.out.html"),	// Basic parsing
	tuple("test/test-2.in.html", "test/test-2.out.html"),	// Parsing <style> and <script>
	tuple("test/test-3.in.html", "test/test-3.out.html"),	// Basic macro
	tuple("test/test-4.in.html", "test/test-4.out.html"),	// Basic expressions
	tuple("test/test-5.in.html", "test/test-5.out.html"),	// Macros and includes
	tuple("test/test-6.in.html", "test/test-6.out.html"),	// Shell
	tuple("test/test-7.in.html", "test/test-7.out.html"),	// Expression functions
};


// ----------------------------------- [ Variables ] ---------------------------------------- //


filesystem::path htmlmacro_path = "./bin/html-macro";


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void slurp(int fd, string& out){
	vector<char> buff = vector<char>(4096);
	
	while (true){
		const ssize_t n = read(fd, buff.data(), buff.size());
		if (n > 0)
			out.append(buff.data(), size_t(n));
		else
			return;
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


int exe(const char* inputfilePath, string& out_1, string& out_2){
	assert(inputfilePath != nullptr);
	int p_stdout[2];
	int p_stderr[2];
	
	if (pipe(p_stdout) != 0){
		ERROR("Internal error when creating a pipe.");
		return 1;
	} else if (pipe(p_stderr) != 0){
		close(p_stdout[0]);
		close(p_stdout[1]);
		ERROR("Internal error when creating a pipe.");
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
		
		execl(htmlmacro_path.c_str(), htmlmacro_path.c_str(), inputfilePath, NULL);
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
					if (info[i].fd == p_stdout[0]){
						slurp(info[i].fd, out_1);
					} else if (info[i].fd == p_stderr[0]){
						size_t n = out_2.length();
						slurp(info[i].fd, out_2);
						cerr.write(out_2.data() + n, out_2.length() - n);
						cerr.flush();
						
					}
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
		ERROR("Timeout on file '%s'.", inputfilePath);
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


static bool run(){
	const size_t total = testFiles.size();
	size_t passed = 0;
	
	string result_1;
	string result_2;
	string expected;
	
	for (auto [in, out] : testFiles){
		result_1.clear();
		result_2.clear();
		
		int fd;
		const int err = exe(in, result_1, result_2);
		if (err != 0){
			goto fail;
		} else if (!result_2.empty()){
			goto fail;
		}
		
		// Compare stdout
		fd = open(out, O_RDONLY);
		if (fd >= 0){
			expected.clear();
			slurp(fd, expected);
			
			if (result_1 != expected){
				close(fd);
				goto fail;
			}
			
			passed++;
			close(fd);
			fprintf(stderr, ANSI_GREEN "PASSED:" ANSI_RESET" '%s' ~ '%s'\n", in, out);
			continue;
		} else {
			ERROR("Could not open expected result file '%s'.", out);
			goto fail;
		}
		
		fail:
		fprintf(stderr, ANSI_RED "FAIL:" ANSI_RESET" '%s' ~ '%s'\n", in, out);
		continue;
	}
	
	if (passed == total){
		INFO("Passed " ANSI_GREEN "%ld" ANSI_RESET "/" ANSI_GREEN "%ld" ANSI_RESET " tests.", passed, total);
	} else {
		INFO("Passed " ANSI_YELLOW "%ld" ANSI_RESET "/" ANSI_GREEN "%ld" ANSI_RESET " tests.", passed, total);
	}
	
	return true;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	const char* prog_cpath = getenv(ENV_HTML_MACRO);
	if (prog_cpath != nullptr){
		htmlmacro_path = prog_cpath;
	}
	
	if (!filesystem::exists(htmlmacro_path)){
		ERROR("Program '%s' does not exist.\n", htmlmacro_path.c_str());
		return 1;
	}
	
	if (!run()){
		return 1;
	}
	
	return 0;
}


// ------------------------------------------------------------------------------------------ //