#include "MacroEngine.hpp"
#include "DEBUG.hpp"

using namespace std;
using namespace pugi;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void log(const char* str, const VariableMap& vars, FILE* out, const char* fmt){
	size_t len;
	if (!hasInterpolation(str, &len)){
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


void MacroEngine::info(const pugi::xml_node op){
	for (const xml_node child : op){
		switch (child.type()){
			case xml_node_type::node_cdata:
			case xml_node_type::node_pcdata:
				log(child.value(), variables, stdout, ANSI_BOLD "INFO: " ANSI_RESET "%s" "\n");
			default:
				break;
		}
	}
}


void MacroEngine::warn(const pugi::xml_node op){
	for (const xml_node child : op){
		switch (child.type()){
			case xml_node_type::node_cdata:
			case xml_node_type::node_pcdata:
				log(child.value(), variables, stderr, ANSI_BOLD ANSI_YELLOW "WARN: " ANSI_RESET "%s" "\n");
			default:
				break;
		}
	}
}


void MacroEngine::error(const pugi::xml_node op){
	for (const xml_node child : op){
		switch (child.type()){
			case xml_node_type::node_cdata:
			case xml_node_type::node_pcdata:
				log(child.value(), variables, stderr, ANSI_BOLD ANSI_RED "ERROR: " ANSI_RESET "%s" "\n");
			default:
				break;
		}
	}
}


// ------------------------------------------------------------------------------------------ //