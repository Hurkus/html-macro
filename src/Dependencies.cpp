#include <cassert>
#include <set>
#include <queue>
#include <regex>
#include <fstream>

#include "fs.hpp"
#include "Paths.hpp"
#include "Debug.hpp"

using namespace std;


#define P(s)	ANSI_PURPLE s ANSI_RESET


// ----------------------------------- [ Constants ] ---------------------------------------- //


// <INCLUDE name="value" name='value'>
static const regex reg_include = regex(
	"<INCLUDE"
	"(?:"							// Multiple attributes
		"\\s+[-_:a-zA-Z]+"			// Attr name
		"(?:"						// optional value
			"\\s*=\\s*"
			"(?:"
				"\"[^\"]*?\""		// Double quote value ""
				"|"
				"'[^']*?'"			// Single quote value ''
			")"
		")*"
	")+?"
	"\\s*/?>"
);


// SRC="..."
static const regex reg_src = regex(
	"\\s"
	"SRC"
	"\\s*=\\s*"
	"\"([^\"{}\n]+)\""
);


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool extract_sources(const filepath& path, string& buff, vector<string>& paths){
	const cregex_iterator end = cregex_iterator();
	
	if (!fs::readFile(path, buff)){
		return false;
	}
	
	// Iterate over <INCLUDE/> macros
	cregex_iterator ri = cregex_iterator(buff.begin().base(), buff.end().base(), reg_include);
	while (ri != end){
		const cmatch& m = *ri;
		const string_view m_str = string_view(m[0].first, m[0].second);
		
		// Iterate over SRC="..." attributes
		cregex_iterator rii = cregex_iterator(m_str.begin(), m_str.end(), reg_src);
		while (rii != end){
			const cmatch& mm = *rii;
			const string_view src = string_view(mm[1].first, mm[1].second);
			
			paths.emplace_back(src);
			
			rii++;
		}
		
		ri++;
	}
	
	return true;
}


static bool process_file(const filepath& file, string& buff, set<filepath>& paths){
	const filepath dir = file.parent_path();
	vector<string> unresolved_paths;
	
	buff.clear();
	if (!extract_sources(file, buff, unresolved_paths)){
		return false;
	}
	
	for (size_t i = 0 ; i < unresolved_paths.size() ; i++){
		filepath path = unresolved_paths[i];
		
		if (!Paths::resolve(path, dir)){
			continue;
		}
		
		auto ep = paths.emplace(move(path));
		const filepath& p = *ep.first;
		process_file(p, buff, paths);
	}
	
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool printDependencies(const char* mainPath){
	set<filepath> paths;
	string buff;
	
	const filepath file = mainPath;
	if (!process_file(file, buff, paths)){
		if (!fs::exists(file))
			ERROR("Input file " P("`%s`") " not found.", file.c_str())
		else
			ERROR("Failed to open input file " P("`%s`") ".", file.c_str())
		return false;
	}
	
	// Print all paths
	for (const filepath& p : paths){
		if (p != file)
			printf("%s\n", p.c_str());
	}
	
	return true;
}


// ------------------------------------------------------------------------------------------ //