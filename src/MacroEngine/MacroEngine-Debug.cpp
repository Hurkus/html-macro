#include "MacroEngine.hpp"
#include "MacroEngine-Common.hpp"

using namespace std;
using namespace pugi;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void _log(const char* str, bool interp, const VariableMap& vars, FILE* out, const char* fmt){
	size_t len;
	if (!interp || !hasInterpolation(str, &len)){
		fprintf(out, fmt, str);
		return;
	}
	
	// Interpolate
	string buff = {};
	buff.append(str, len);
	interpolate(str + len, vars, buff);
	
	fprintf(out, fmt, buff.c_str());
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngineObject::info(const xml_node op){
	bool intrp = this->interpolateText;
	
	for (const xml_attribute attr : op.attributes()){
		string_view name = attr.name();
		
		if (name == "IF"){
			if (!_attr_if(*this, op, attr))
				return;
		} else if (name == "INTERPOLATE"){
			_attr_interpolate(*this, op, attr, intrp);
		} else {
			_attr_ignore(op, attr);
		}
		
	}
	
	for (const xml_node child : op){
		switch (child.type()){
			case xml_node_type::node_cdata:
			case xml_node_type::node_pcdata:
				_log(child.value(), intrp, variables, stdout, ANSI_BOLD "INFO: " ANSI_RESET "%s" "\n");
			default:
				break;
		}
	}
	
}


void MacroEngineObject::warn(const pugi::xml_node op){
	bool intrp = this->interpolateText;
	
	for (const xml_attribute attr : op.attributes()){
		string_view name = attr.name();
		
		if (name == "IF"){
			if (!_attr_if(*this, op, attr))
				return;
		} else if (name == "INTERPOLATE"){
			_attr_interpolate(*this, op, attr, intrp);
		} else {
			_attr_ignore(op, attr);
		}
		
	}
	
	for (const xml_node child : op){
		switch (child.type()){
			case xml_node_type::node_cdata:
			case xml_node_type::node_pcdata:
				_log(child.value(), intrp, variables, stderr, ANSI_BOLD ANSI_YELLOW "WARN: " ANSI_RESET "%s" "\n");
			default:
				break;
		}
	}
	
}


void MacroEngineObject::error(const pugi::xml_node op){
	bool intrp = this->interpolateText;
	
	for (const xml_attribute attr : op.attributes()){
		string_view name = attr.name();
		
		if (name == "IF"){
			if (!_attr_if(*this, op, attr))
				return;
		} else if (name == "INTERPOLATE"){
			_attr_interpolate(*this, op, attr, intrp);
		} else {
			_attr_ignore(op, attr);
		}
		
	}
	
	for (const xml_node child : op){
		switch (child.type()){
			case xml_node_type::node_cdata:
			case xml_node_type::node_pcdata:
				_log(child.value(), intrp, variables, stderr, ANSI_BOLD ANSI_RED "ERROR: " ANSI_RESET "%s" "\n");
			default:
				break;
		}
	}
	
}


// ------------------------------------------------------------------------------------------ //