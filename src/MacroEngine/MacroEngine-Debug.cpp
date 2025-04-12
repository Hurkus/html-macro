#include "MacroEngine.hpp"
#include "DEBUG.hpp"

using namespace std;
using namespace pugi;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::info(const pugi::xml_node op){
	for (const xml_node child : op){
		switch (child.type()){
			case xml_node_type::node_cdata:
			case xml_node_type::node_pcdata:
				INFO(ANSI_BOLD "INFO: " ANSI_RESET "%s", child.value());
				break;
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
				INFO(ANSI_BOLD ANSI_YELLOW "WARN: " ANSI_RESET "%s", child.value());
				break;
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
				INFO(ANSI_BOLD ANSI_RED "ERROR: " ANSI_RESET "%s", child.value());
				break;
			default:
				break;
		}
	}
}


// ------------------------------------------------------------------------------------------ //