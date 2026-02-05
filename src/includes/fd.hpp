#include <unistd.h>


namespace fs {
	struct FileDesc;
}


struct fs::FileDesc {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	int fd = -1;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	FileDesc() = default;
	
	FileDesc(int fd) : fd{fd} {}
	
	FileDesc(FileDesc&& o) : fd{o.fd} {
		o.fd = -1;
	}
	
public:
	~FileDesc(){
		close();
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void close() noexcept {
		if (fd >= 0){
			::close(fd);
			fd = -1;
		}
	}
	
	bool isOpen() const {
		return fd >= 0;
	}
	
// ----------------------------------- [ Operators ] ---------------------------------------- //
public:
	FileDesc& operator=(FileDesc&& o){
		int x = o.fd;
		o.fd = fd;
		fd = x;
		return *this;
	}
	
	operator int(){
		return fd;
	}
	
// ------------------------------------------------------------------------------------------ //
};



namespace fs {
// ----------------------------------- [ Functions ] ---------------------------------------- //
	

inline bool pipe(FileDesc& fd0, FileDesc& fd1) noexcept {
	int fd[2];
	if (::pipe(fd) == 0){
		fd0 = FileDesc(fd[0]);
		fd1 = FileDesc(fd[1]);
		return true;
	}
	return false;
}


// ------------------------------------------------------------------------------------------ //
}
