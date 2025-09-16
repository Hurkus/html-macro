#include "Macro.hpp"
#include <unordered_map>
#include <fstream>

#include "Debug.hpp"
#include "DebugSource.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;

// ----------------------------------- [ Variables ] ---------------------------------------- //


static unordered_map<string_view,shared_ptr<Macro>> macroFileCache;
static unordered_map<string_view,shared_ptr<Macro>> macroNameCache;


// ----------------------------------- [ Functions ] ---------------------------------------- //


static linepos findLine(const Document& doc, const char* p){
	linepos l = {};
	
	if (doc.buffer != nullptr){
		string_view buff = string_view(*doc.buffer);
		l = findLine(buff.begin(), buff.end(), p);
	}
	
	l.file = (doc.srcFile != nullptr) ? doc.srcFile->c_str() : nullptr;
	return l;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool registerMacro(const Macro& parent, Node&& macro){
	// Find name
	Attr* a = macro.attribute;
	while (a != nullptr && a->name() != "NAME"){
		a = a->next;
	}
	
	if (a == nullptr){
		warn_missing_attr(macro, "NAME");
		return false;
	}
	
	// Create new macro by moving <MACRO> node to new document.
	unique_ptr<Macro> m = make_unique<Macro>();
	m->name = a->value();
	m->srcDir = parent.srcDir;
	Node& root = m->doc;
	
	// Transfer node
	m->doc.buffer = parent.doc.buffer;
	m->doc.srcFile = parent.doc.srcFile;
	swap(macro.options, root.options);
	swap(macro.value_len, root.value_len);
	swap(macro.value_p, root.value_p);
	swap(macro.attribute, root.attribute);
	swap(macro.child, root.child);
	
	for (Node* child = root.child ; child != nullptr ; child = child->next){
		child->parent = &root;
	}
	
	// Register new macro
	macroNameCache[m->name] = move(m);
	return true;
}


static void err(const Document& doc, const ParseResult& res){
	const char* msg = html::errstr(res.status);
	
	switch (res.status){
		case ParseStatus::IO:
		case ParseStatus::MEMORY:
			ERROR("%s", msg);
			break;
		default:
			linepos pos = findLine(doc, res.pos.begin());
			HERE(print(pos));
			::error("%s", msg);
			printf("%s\n", getCodeView(pos, res.pos, ANSI_RED).c_str());
			break;
	}
	
}


static Macro* parseFile(string&& path){
	unique_ptr<Macro> macro = make_unique<Macro>();
	
	// Parse
	ParseResult res = macro->doc.parseFile(move(path));
	switch (res.status){
		case ParseStatus::OK:
			break;
		case ParseStatus::IO:
			if (!filesystem::exists(path)){
				ERROR("Failed to read file '%s'. No such file exists.", path.c_str());
			} else {
				ERROR("Failed to read file '%s'.", path.c_str());
			}
			return nullptr;
		default:
			err(macro->doc, res);
			return nullptr;
	}
	
	assert(macro->doc.srcFile != nullptr);
	filepath filePath = filepath(*macro->doc.srcFile);
	macro->srcDir = make_shared<filepath>(move(filePath.remove_filename()));
	
	// Extract all <MACRO> nodes.
	for (Node* m : res.macros){
		assert(m != nullptr && m->parent != nullptr);
		registerMacro(*macro, move(*m));
		Node::del(m);
	}
	
	// Store macro
	string_view key = string_view(*macro->doc.srcFile);
	const auto& p = macroFileCache.emplace(key, move(macro));
	return p.first->second.get();
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


const Macro* Macro::get(string_view name){
	auto p = macroNameCache.find(name);
	if (p != macroNameCache.end())
		return p->second.get();
	return nullptr;
}


const Macro* Macro::loadFile(const filepath& path, bool resolve){
	if (resolve){
		filepath _path = MacroEngine::resolve(path);
		string_view file_view = _path.c_str();
		
		// Check if cached files.
		auto p = macroFileCache.find(file_view);
		if (p != macroFileCache.end()){
			return p->second.get();
		}
		
		// Parse new file
		return parseFile(move(_path).string());
	}
	else {
		string_view file_view = path.c_str();
		
		// Check if cached files.
		auto p = macroFileCache.find(file_view);
		if (p != macroFileCache.end()){
			return p->second.get();
		}
		
		return parseFile(path.string());
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


char* Macro::loadRawFile(const filepath& path, size_t& out_len){
	ifstream in = ifstream(path);
	if (!in){
		return nullptr;
	}
	
	if (!in.seekg(0, ios_base::end)){
		return nullptr;
	}
	
	const streampos pos = in.tellg();
	if (pos < 0 || !in.seekg(0, ios_base::beg)){
		return nullptr;
	}
	
	const size_t size = size_t(pos);
	char* buff = html::newStr(size + 1);
	size_t total = 0;
	
	while (total < size && in.read(buff + total, size - total)){
		const streamsize n = in.gcount();
		if (n <= 0){
			html::del(buff);
			return nullptr;
		}
		
		total += size_t(n);
	}
	
	buff[total] = 0;
	out_len = total;
	return buff;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void Macro::clearCache(){
	macroFileCache.clear();
	macroNameCache.clear();
}


// ------------------------------------------------------------------------------------------ //