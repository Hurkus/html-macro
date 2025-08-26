#include <memory>
#include <vector>
#include <filesystem>


namespace MacroEngine {
	using filepath = std::filesystem::path;
	
	extern std::shared_ptr<filepath> cwd;
	extern std::vector<filepath> paths;
	
	filepath resolve(const filepath& path);
};
