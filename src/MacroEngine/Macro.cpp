#include "Macro.hpp"
#include "str_map.hpp"
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


static str_map<shared_ptr<Macro>> macroFileCache;
static str_map<shared_ptr<Macro>> macroNameCache;


// ----------------------------------- [ Functions ] ---------------------------------------- //


Macro::Type Macro::getType(const filepath& path){
	const string ext = path.extension().string();
	if (ext == ".html"sv || ext == ".svg"sv || ext == ".xml"sv)
		return Type::HTML;
	else if (ext == ".css"sv)
		return Type::CSS;
	else if (ext == ".js"sv)
		return Type::JS;
	else
		return Type::TXT;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static unique_ptr<Document> split(Document& srcDoc, Node* node){
	assert(node != nullptr);
	
	unique_ptr<Document> newDoc = make_unique<Document>();
	srcDoc.extractChild(node);
	assert(node->parent == nullptr);
	assert(node->next == nullptr);
	
	// Clone shared storage
	newDoc->buffer = shared_ptr(srcDoc.buffer);
	newDoc->nodeAlloc = shared_ptr(srcDoc.nodeAlloc);
	newDoc->attrAlloc = shared_ptr(srcDoc.attrAlloc);
	newDoc->charAlloc = shared_ptr(srcDoc.charAlloc);
	
	// Clone node properties
	newDoc->options   = node->options;
	newDoc->type      = NodeType::ROOT;
	newDoc->value_len = node->value_len;
	newDoc->value_p   = node->value_p;
	newDoc->attribute = node->attribute;
	newDoc->parent    = nullptr;
	newDoc->child     = node->child;
	newDoc->next      = nullptr;
	
	// Adjust children
	for (Node* child = newDoc->child ; child != nullptr ; child = child->next){
		child->parent = newDoc.get();
	}
	
	assert(srcDoc.nodeAlloc);
	srcDoc.nodeAlloc->dealloc(node);
	return newDoc;
}


static bool registerChildMacro(const Macro& srcMacro, unique_ptr<Document>&& doc){
	assert(doc != nullptr);
	const Attr* name_attr = nullptr;
	
	// Find name
	for (const Attr* attr = doc->attribute ; attr != nullptr ; attr = attr->next){
		if (attr->name() == "NAME"sv){
			if (name_attr == nullptr)
				name_attr = attr;
			else
				HERE(warn_duplicate_attr(srcMacro, *name_attr, *attr));
			continue;
		}
	}
	
	if (name_attr == nullptr){
		HERE(warn_missing_attr(srcMacro, *doc, "NAME"));
		return false;
	} else if (name_attr->value_len <= 0){
		HERE(error_missing_attr_value(srcMacro, *name_attr));
		return false;
	} else if (name_attr->options % NodeOptions::SINGLE_QUOTE){
		HERE(warn_expected_attr_double_quote(srcMacro, *name_attr));
	}
	
	unique_ptr<Macro> m = make_unique<Macro>();
	m->name = name_attr->value();
	m->srcFile = shared_ptr(srcMacro.srcFile);
	m->srcDir = shared_ptr(srcMacro.srcDir);
	m->type = Macro::Type::HTML;
	m->html = move(doc);
	
	// Register new macro
	macroNameCache.insert(m->name, move(m));
	return true;
}


bool Macro::parseHTML(){
	if (this->html != nullptr){
		return true;
	} else if (this->txt == nullptr){
		return false;
	}
	
	unique_ptr<Document> doc = make_unique<Document>();
	ParseResult res = doc->parse(this->txt);
	
	// Report parsing error
	switch (res.status){
		case ParseResult::Status::OK:
			break;

		case ParseResult::Status::MEMORY:
			ERROR("%s", res.msg());
			break;
		
		default: {
			string_view buff = string_view(*doc->buffer);
			linepos pos = findLine(buff.begin(), buff.end(), res.mark.data());
			pos.file = (this->srcFile != nullptr) ? this->srcFile->c_str() : "-";
			
			HERE(print(pos));
			LOG_STDERR(ANSI_BOLD ANSI_RED "error: " ANSI_RESET "%s\n", res.msg());
			printCodeView(pos, res.mark, ANSI_RED);
			return false;
		}
		
	}
	
	// Extract and register all child <MACRO> nodes.
	for (Node* mnode : res.macros){
		assert(mnode != nullptr && mnode->parent != nullptr);
		unique_ptr<Document> mdoc = split(*doc, mnode);
		registerChildMacro(*this, move(mdoc));
	}
	
	this->html = move(doc);
	return true;
};


bool Macro::parse(Type type){
	switch (type){
		case Type::HTML:
			return parseHTML();
		default:
			return this->txt != nullptr;
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


shared_ptr<Macro> MacroCache::get(string_view name) noexcept {
	const shared_ptr<Macro>* p = macroNameCache.get(name);
	if (p != nullptr)
		return *p;
	return nullptr;
}


shared_ptr<Macro> MacroCache::load(filepath& path){
	if (!Paths::resolve(path)){
		return nullptr;
	}
	
	const string_view path_sv = path.c_str();
	
	// Check if cached files.
	const shared_ptr<Macro>* p = macroFileCache.get(path_sv);
	if (p != nullptr){
		return *p;
	}
	
	// Load new file
	unique_ptr<string> txt = make_unique<string>();
	if (!fs::readFile(path, *txt)){
		return nullptr;
	}
	
	unique_ptr<Macro> macro = make_unique<Macro>();
	macro->name = path.filename().string();
	macro->srcFile = make_shared<filepath>(path);
	macro->srcDir = make_shared<filepath>(path.parent_path());
	macro->type = Macro::getType(path);
	macro->txt = move(txt);
	
	// Store macro
	string_view key = string_view(macro->srcFile->c_str());
	return macroFileCache.insert(key, move(macro));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroCache::clear(){
	macroFileCache.clear();
	macroNameCache.clear();
}


// ------------------------------------------------------------------------------------------ //