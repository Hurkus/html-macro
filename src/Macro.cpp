#include "Macro.hpp"
#include <cstring>

#include <iostream>
#include "DEBUG.hpp"

using namespace std;
using namespace pugi;


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void findMacroDefs(const xml_node root, vector<xml_node>& out_nodes){
	if (strcmp(root.name(), "MACRO") == 0){
		out_nodes.emplace_back(root);
	}
	
	for (const xml_node child : root.children()){
		findMacroDefs(child, out_nodes);
	}
	
}


static bool parseMacroDef(xml_node node, Macro& macro){
	macro.name = node.attribute("NAME").value();
	return !macro.name.empty();
}


static unique_ptr<Macro> extractMacroDef(xml_node node){
	unique_ptr<Macro> m = make_unique<Macro>();
	
	// Parse attributes
	if (!parseMacroDef(node, *m)){
		return nullptr;
	}
	
	// Move node to new document tree
	if (!m->root.append_copy(node)){
		node.parent().remove_child(node);
		return nullptr;
	} else {
		node.parent().remove_child(node);
	}
	
	xml_node def = m->root.first_child();
	
	// Ascend children
	while (true){
		xml_node child = def.first_child();
		if (!child.empty())
			m->root.append_move(child);
		else
			break;
	}
	
	m->root.remove_child(def);
	return m;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


vector<unique_ptr<Macro>> XHTMLFile::convertToMacroSet(){
	// Find all MACRO tags.
	vector<xml_node> nodes;
	findMacroDefs(doc, nodes);
	
	vector<unique_ptr<Macro>> macros;
	macros.reserve(macros.size() + nodes.size() + 1);
	shared_ptr<filesystem::path> src = make_shared<filesystem::path>(move(this->path));
	
	// Create new macro sub-documents and remove from original document.
	for (int i = 0 ; i < int(nodes.size()) ; i++){
		unique_ptr<Macro> m = extractMacroDef(nodes[i]);
		if (m != nullptr){
			m->srcFile = src;
			macros.push_back(move(m));
		}
	}
	
	// Add self
	Macro& m = *macros.emplace_back(make_unique<Macro>());
	m.root = move(this->doc);
	m.srcFile = move(src);
	m.name = *m.srcFile;
	
	return macros;
}


// ------------------------------------------------------------------------------------------ //