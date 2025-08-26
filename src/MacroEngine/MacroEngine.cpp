#include "MacroEngine.hpp"
#include <cstring>

#include "MacroEngine-Common.hpp"

using namespace std;
using namespace pugi;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::call(const char* name, xml_node dst){
	if (name == nullptr || name[0] == 0){
		ERROR("CALL: Missing macro name.");
		return;
	}
	
	shared_ptr<Macro> macro = getMacro(name);
	if (macro == nullptr){
		WARN("CALL: Macro '%s' not found.", name);
		return;
	}
	
	exec(*macro, dst);
}


void MacroEngine::call(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	xml_attribute name_attr;
	
	for (const xml_attribute attr : op.attributes()){
		string_view name = attr.name();
		
		if (name == "NAME"){
			name_attr = attr;
		} else if (name == "IF"){
			if (!_attr_if(*this, op, attr))
				return;
		} else {
			_attr_ignore(op, attr);
		}
		
	}
	
	if (!name_attr.empty()){
		call(name_attr.value(), dst);
	} else {
		WARN("CALL: Missing 'NAME' attribute.");
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::include(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	xml_attribute src_attr;
	
	for (const xml_attribute attr : op.attributes()){
		string_view name = attr.name();
		
		if (name == "SRC"){
			src_attr = attr;
		} else if (name == "IF"){
			if (!_attr_if(*this, op, attr))
				return false;
		} else {
			_attr_ignore(op, attr);
		}
		
	}
	
	if (src_attr.empty()){
		_attr_missing(op, "SRC");
		return false;
	}
	
	// Modify relative path to current macro file path.
	filesystem::path path;
	try {
		path = src_attr.value();
		
		if (!path.is_absolute()){
			assert(currentMacro != nullptr);
			if (currentMacro == nullptr || currentMacro->srcFile == nullptr){
				ERROR("%s: Missing macro relative path.", op.name());
				return false;
			}
			
			filesystem::path dir = *currentMacro->srcFile;
			dir.remove_filename();
			path = dir / filesystem::proximate(move(path), dir);
		}
		
	} catch (const exception& e){
		ERROR("%s: Failed to construct path '%s'.", op.name(), src_attr.value());
		return false;
	}
	
	// Fetch and execute macro
	shared_ptr<Macro> macro = loadFile(path);
	if (macro == nullptr){
		return false;
	}
	
	exec(*macro, dst);
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::runChildren(const xml_node parent, xml_node dst){
	assert(parent.root() != dst.root());
	auto _branch = this->branch;
	this->branch = nullptr;
	
	for (const xml_node mchild : parent.children()){
		run(mchild, dst);
	}
	
	this->branch = _branch;
}


void MacroEngine::run(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	
	// Not tag
	xml_node_type type = op.type();
	if (type != xml_node_type::node_element){
		if (type == xml_node_type::node_pcdata)
			text(op.value(), dst);
		else
			dst.append_copy(op);
		return;
	}
	
	// Check for macro
	const char* cname = op.name();
	if (isupper(cname[0])){
		string_view name = cname;
		
		if (name == "SET"){
			set(op);
		} else if (name == "IF"){
			branch_if(op, dst);
		} else if (name == "ELSE-IF"){
			branch_elif(op, dst);
		} else if (name == "ELSE"){
			branch_else(op, dst);
		} else if (name == "FOR"){
			loop_for(op, dst);
		} else if (name == "WHILE"){
			loop_while(op, dst);
		}
		
		else if (name == "SET-ATTR"){
			setAttr(op, dst);
		} else if (name == "GET-ATTR"){
			getAttr(op, dst);
		} else if (name == "DEL-ATTR"){
			delAttr(op, dst);
		} else if (name == "SET-TAG"){
			setTag(op, dst);
		} else if (name == "GET-TAG"){
			getTag(op, dst);
		}
		
		else if (name == "CALL"){
			call(op, dst);
		} else if (name == "INCLUDE"){
			include(op, dst);
		}  else if (name == "SHELL"){
			shell(op, dst);
		}
		
		else if (name == "INFO"){
			info(op);
		} else if (name == "WARN"){
			warn(op);
		} else if (name == "ERROR"){
			error(op);
		} else {
			WARN("%s: Unknown macro treated as regular HTML tag.", cname);
			tag(op, dst);
		}
		
		return;
	}
	
	// Regular tag
	tag(op, dst);
}


void MacroEngine::exec(const Macro& macro, xml_node dst){
	const bool _interp = this->interpolateText;
	this->interpolateText = true;
	
	const optbool _branch = this->branch;
	this->branch = nullptr;
	
	const Macro* _currentMacro = this->currentMacro;
	this->currentMacro = &macro;
	
	runChildren(macro.root, dst);
	
	this->currentMacro = _currentMacro;
	this->branch = _branch;
	this->interpolateText = _interp;
}


// ------------------------------------------------------------------------------------------ //