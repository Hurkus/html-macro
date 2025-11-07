#include "Macro.hpp"
#include <unordered_map>

#include "fs.hpp"
#include "Debug.hpp"
#include "DebugSource.hpp"

using namespace std;
using namespace html;

/*
Disaster *almost* occurs with <INCLUDE/> macros, since they trigger macro shadowing.

The main `html::Document` generally references bits of text from other macros using raw pointers.
Overwritten macros release their share of the buffer, thus (likely) invalidating `html::Node`s from the main document.

This currently isn't a problem, because all macros from the `macroNameCache` originate from a root macro in `macroFileCache`.
Root macros cannot get shadowed (they are unique to their file path).
Name macros (`macroNameCache`) originate from root macros (`macroFileCache`) and share the buffer (`shared_ptr<string>`).
This means that all raw pointers remain valid, since at least 1 macro (root) keeps the buffer alive untill the end of the process.
*/

// ----------------------------------- [ Variables ] ---------------------------------------- //


static unordered_map<string_view,shared_ptr<Macro>> macroFileCache;
static unordered_map<string_view,shared_ptr<Macro>> macroNameCache;


// ----------------------------------- [ Functions ] ---------------------------------------- //


Macro::Type Macro::getType(const filepath& path){
	const filepath ext = path.extension();
	if (ext == ".html"sv)
		return Type::HTML;
	else if (ext == ".css"sv)
		return Type::CSS;
	else if (ext == ".js"sv)
		return Type::JS;
	else
		return Type::TXT;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool registerChildMacro(const Macro& parent, Node&& macroNode){
	const Attr* name_attr = nullptr;
	
	// Find name
	for (const Attr* attr = macroNode.attribute ; attr != nullptr ; attr = attr->next){
		if (attr->name() == "NAME"sv){
			if (name_attr == nullptr)
				name_attr = attr;
			else
				HERE(warn_duplicate_attr(macroNode, *name_attr, *attr));
			continue;
		}
	}
	
	if (name_attr == nullptr){
		HERE(warn_missing_attr(macroNode, "NAME"));
		return false;
	} else if (name_attr->value_len <= 0){
		HERE(error_missing_attr_value(macroNode, *name_attr));
		return false;
	} else if (name_attr->options % NodeOptions::SINGLE_QUOTE){
		HERE(warn_attr_single_quote(macroNode, *name_attr));
	}
	
	// Transfer node to root of new document
	unique_ptr<Document> newDoc = make_unique<Document>();
	{
		assert(parent.html != nullptr);
		newDoc->buffer = shared_ptr(parent.html->buffer);
		newDoc->srcFile = shared_ptr(parent.html->srcFile);
		
		Node& root = *newDoc;
		swap(macroNode.options,   root.options);
		swap(macroNode.value_len, root.value_len);
		swap(macroNode.value_p,   root.value_p);
		swap(macroNode.attribute, root.attribute);
		swap(macroNode.child,     root.child);
		
		for (Node* child = root.child ; child != nullptr ; child = child->next){
			child->parent = &root;
		}
		
		assert(macroNode.child == nullptr);
	}
	
	unique_ptr<Macro> m = make_unique<Macro>();
	m->name = name_attr->value();
	m->srcFile = shared_ptr(parent.srcFile);
	m->srcDir = shared_ptr(parent.srcDir);
	m->type = Macro::Type::HTML;
	m->html = move(newDoc);
	
	// Register new macro
	macroNameCache.emplace(m->name, move(m));
	return true;
}


bool Macro::parseHTML(){
	if (this->html != nullptr){
		return true;
	} else if (this->txt == nullptr){
		return false;
	}
	
	unique_ptr<html::Document> doc = make_unique<html::Document>();
	ParseResult res = doc->parseBuff(this->txt);
	
	// Report parsing error
	switch (res.status){
		case ParseStatus::OK:
			break;

		case ParseStatus::IO:
		case ParseStatus::MEMORY:
			ERROR("%s", html::errstr(res.status));
			break;
		
		default: {
			string_view buff = string_view(*doc->buffer);
			linepos pos = findLine(buff.begin(), buff.end(), res.pos.data());
			pos.file = (this->srcFile != nullptr) ? this->srcFile->c_str() : "-";
			
			HERE(print(pos));
			printErrTag();
			LOG_STDERR("%s\n", html::errstr(res.status));
			printCodeView(pos, res.pos, ANSI_RED);
			return false;
		}
		
	}
	
	doc->srcFile = this->srcFile;
	this->html = move(doc);
	
	// Extract and register all child <MACRO> nodes.
	for (Node* m : res.macros){
		assert(m != nullptr && m->parent != nullptr);
		
		registerChildMacro(*this, move(*m));
		bool remd = m->parent->removeChild(m);
		
		assert(remd);
	}
	
	return true;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool Macro::parse(){
	switch (type){
		case Type::HTML:
			return parseHTML();
		default:
			return this->txt != nullptr;
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Macro* MacroCache::get(string_view name) noexcept {
	auto p = macroNameCache.find(name);
	if (p != macroNameCache.end())
		return p->second.get();
	return nullptr;
}


Macro* MacroCache::load(filepath& path){
	if (!Paths::resolve(path)){
		return nullptr;
	}
	
	const string_view path_sv = path.c_str();
	
	// Check if cached files.
	auto p = macroFileCache.find(path_sv);
	if (p != macroFileCache.end()){
		return p->second.get();
	}
	
	// Load new file
	unique_ptr<string> txt = make_unique<string>();
	if (!fs::readFile(path, *txt)){
		return nullptr;
	}
	
	shared_ptr<Macro> macro = make_shared<Macro>();
	macro->name = path.filename().string();
	macro->srcFile = make_shared<filepath>(path);
	macro->srcDir = make_shared<filepath>(path.remove_filename());
	macro->type = Macro::getType(*macro->srcFile);
	macro->txt = move(txt);
	
	// Store macro
	string_view key = string_view(macro->srcFile->c_str());
	const auto& x = macroFileCache.emplace(key, move(macro));
	return x.first->second.get();
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroCache::clear(){
	macroFileCache.clear();
	macroNameCache.clear();
}


// ------------------------------------------------------------------------------------------ //