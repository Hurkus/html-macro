#include "Macro.hpp"
#include "string_map.hpp"
#include "Paths.hpp"

#include "Debug.hpp"
#include "html-debug.hpp"

using namespace std;
using namespace pugi;
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
	m->doc.buffer_owned = parent.doc.buffer_owned;
	m->doc.buffer_unowned = parent.doc.buffer_unowned;
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


static Macro* loadFile(string&& path){
	unique_ptr<Macro> macro = make_unique<Macro>();
	ParseResult res = macro->doc.parseFile(move(path));
	
	switch (res.status){
		case ParseStatus::OK:
			break;
		default: {
			const char* file = macro->doc.file();
			long row = macro->doc.row(res.pos);
			long col = macro->doc.col(res.pos);
			const char* msg = html::errstr(res.status);
			ERROR(ANSI_BOLD "%s:%ld:%ld:" ANSI_RESET " %s", file, row, col, msg);
			return nullptr;
		}
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


// ----------------------------------- [ Functions ] ---------------------------------------- //


const Macro* Macro::get(string_view name){
	auto p = macroNameCache.find(name);
	if (p != macroNameCache.end())
		return p->second.get();
	return nullptr;
}


const Macro* Macro::load(string_view path){
	string file = resolve(path).string();
	
	// Check if cached files.
	auto p = macroFileCache.find(string_view(file));
	if (p != macroFileCache.end()){
		return p->second.get();
	}
	
	// Parse new file
	return loadFile(move(file));
}


// ------------------------------------------------------------------------------------------ //