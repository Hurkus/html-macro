#include "Macro.hpp"
#include <unordered_map>

#include "Paths.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;

// ----------------------------------- [ Variables ] ---------------------------------------- //


static unordered_map<string_view,shared_ptr<Macro>> macroFileCache;
static unordered_map<string_view,shared_ptr<Macro>> macroNameCache;


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
	
	// Transfer node
	m->doc.buffer = parent.doc.buffer;
	m->doc.srcFile = parent.doc.srcFile;
	swap(macro.options, m->doc.options);
	swap(macro.value_len, m->doc.value_len);
	swap(macro.value_p, m->doc.value_p);
	swap(macro.child, m->doc.child);
	swap(macro.attribute, m->doc.attribute);
	
	// Register new macro
	macroNameCache[m->name] = move(m);
	return true;
}


static void err(const Document& doc, const ParseResult& res){
	const char* file = (doc.srcFile == nullptr) ? "buffer" : doc.srcFile->c_str();
	const char* msg = html::errstr(res.status);
	
	switch (res.status){
		case ParseStatus::IO:
		case ParseStatus::MEMORY:
			ERROR(ANSI_BOLD "%s:" ANSI_RESET " %s", file, msg);
			break;
		default:
			long row = doc.row(res.pos);
			long col = doc.col(res.pos);
			ERROR(ANSI_BOLD "%s:%ld:%ld:" ANSI_RESET " %s", file, row, col, msg);
			break;
	}
	
}


static Macro* parseFile(string&& path){
	unique_ptr<Macro> macro = make_unique<Macro>();
	
	// Parse
	ParseResult res = macro->doc.parseFile(move(path));
	if (res.status != ParseStatus::OK){
		err(macro->doc, res);
		return nullptr;
	}
	
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


static unique_ptr<Macro> parseBuffer(shared_ptr<const string>&& buff){
	unique_ptr<Macro> macro = make_unique<Macro>();
	
	// Parse
	ParseResult res = macro->doc.parseBuff(move(buff));
	if (res.status != ParseStatus::OK){
		err(macro->doc, res);
		return nullptr;
	}
	
	// Extract all <MACRO> nodes.
	for (Node* m : res.macros){
		assert(m != nullptr && m->parent != nullptr);
		registerMacro(*macro, move(*m));
		Node::del(m);
	}
	
	return macro;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


const Macro* Macro::get(string_view name){
	auto p = macroNameCache.find(name);
	if (p != macroNameCache.end())
		return p->second.get();
	return nullptr;
}


const Macro* Macro::loadFile(string_view path){
	string file = resolve(path).string();
	
	// Check if cached files.
	auto p = macroFileCache.find(string_view(file));
	if (p != macroFileCache.end()){
		return p->second.get();
	}
	
	// Parse new file
	return parseFile(move(file));
}


unique_ptr<Macro> Macro::loadBuffer(shared_ptr<const string>&& buff){
	return parseBuffer(move(buff));
}


// ------------------------------------------------------------------------------------------ //