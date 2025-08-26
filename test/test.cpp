#include <cassert>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <tuple>
#include <filesystem>

extern "C" {
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
	tuple("test/test-3.in.html", "test/test-3.out.html"),	// Parsing <style> and <script>
	tuple("test/test-4.in.html", "test/test-4.out.html"),	// Testing basic expressions
	tuple("test/test-5.in.html", "test/test-5.out.html"),	// Testing macros and includes
	tuple("test/test-6.in.html", "test/test-6.out.html"),	// Testing shell
};


// ----------------------------------- [ Variables ] ---------------------------------------- //


filesystem::path htmlmacro_path = "./bin/html-macro";


// ----------------------------------- [ Functions ] ---------------------------------------- //


static size_t slurp(int fd, string& out){
	// Array chunks that are concatenated into final string after EOF.
	size_t totalSize = 0;
	vector<vector<char>> buffs = {};
	buffs.reserve(4);
	
	while (true){
		vector<char>& buff = buffs.emplace_back();
		buff.resize(buffs.size() * 4096);	// Each larger than previous.
		
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


int exe(const char* in, string& out){
	assert(in != nullptr);
	
	int p_stdout[2];
	if (pipe(p_stdout) != 0){
		ERROR("Internal error when creating a pipe.");
		return 1;
	}
	
	const pid_t pid = fork();
	
	// Child
	if (pid == 0){
		dup2(p_stdout[1], 1);
		
		close(0);
		close(2);
		close(p_stdout[0]);
		close(p_stdout[1]);
		
		execl(htmlmacro_path.c_str(), htmlmacro_path.c_str(), in, NULL);
		exit(1);
	}
	
	// Parent
	else if (pid > 0){
		close(p_stdout[1]);
		slurp(p_stdout[0], out);
		close(p_stdout[0]);
		
		// Get return status of child.
		int err = 0;
		waitpid(pid, &err, 0);
		return WEXITSTATUS(err);
	}
	
	// Error
	close(p_stdout[0]);
	close(p_stdout[1]);
	return 1;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool run(){
	const size_t total = testFiles.size();
	size_t passed = 0;
	
	string result;
	string expected;
	
	for (auto [in, out] : testFiles){
		result.clear();
		
		int fd;
		const int err = exe(in, result);
		if (err != 0){
			goto fail;
		}
		
		fd = open(out, O_RDONLY);
		if (fd >= 0){
			expected.clear();
			slurp(fd, expected);
			
			if (result != expected){
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