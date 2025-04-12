#include "MacroParser.hpp"
#include <fstream>

#include "DEBUG.hpp"

using namespace std;
using namespace pugi;


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool findRowCol(ifstream& in, int pos, int& row, int& col){
	if (!in.seekg(0)){
		return false;
	}
	
	string line;
	int ln = 0;
	int is = 0;
	int ie = 0;
	
	while (getline(in, line)){
		ln++;
		
		ie = is + line.length() + 1;
		if (ie > pos){
			break;
		}
		
		is = ie;
	}
	
	if (ie > pos){
		row = ln;
		col = pos - is + 1;
		return true;
	}
	
	return false;
}


static bool findRowCol(string_view buff, int pos, int& row, int& col){
	int ln = 1;
	
	const char* const beg = buff.begin();
	const char* const end = buff.end();
	const char* const ppos = beg + pos;
	const char* p = beg;
	const char* nl = beg;
	
	while (p != end && p != ppos){
		if (*p == '\n'){
			ln++;
			nl = p;
		}
		p++;
	}
	
	if (p == ppos){
		row = ln;
		col = p - nl + 1;
		return true;
	}
	
	return false;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool parseBuffer(std::string_view buff, pugi::xml_node dst){
	int flags = pugi::parse_default | pugi::parse_doctype | pugi::parse_fragment;
	xml_parse_result result = dst.append_buffer(buff.data(), buff.length(), flags, pugi::encoding_utf8);
	
	switch (result.status){
		case status_ok:
			return true;
		
		case status_end_element_mismatch: {
			const char* expl = "Some HTML tags must be made explicitly self closing, such as '<img/>', '<meta/>' and others.";
			
			int row, col;
			if (findRowCol(buff, result.offset, row, col) && row > 0)
				errorf("unknown", row, "%s. %s", result.description(), expl);
			else
				errorf("unknown", "%s. %s", result.description(), expl);
			
			return false;
		} break;
		
		case status_unrecognized_tag:
		case status_bad_pi:
		case status_bad_comment:
		case status_bad_cdata:
		case status_bad_doctype:
		case status_bad_pcdata:
		case status_bad_start_element:
		case status_bad_attribute:
		case status_bad_end_element:
		case status_no_document_element: {
			
			int row, col;
			if (findRowCol(buff, result.offset, row, col) && row > 0){
				errorf("unknown", row, "%s.", result.description());
				return false;
			}
			
			goto default_err;
		} break;
		
		default:{
			default_err:
			errorf("unknown", "%s.", result.description());
			return false;
		} break;
		
	}
	
	return false;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool parseFile(const char* file, xml_document& doc){
	ifstream in = ifstream(file);
	if (!in){
		ERROR_L1("MacroParser: Failed to open file '%s'.", file);
		return false;
	}
	
	xml_parse_result result = doc.load(in, pugi::parse_default | pugi::parse_doctype, pugi::encoding_utf8);
	switch (result.status){
		case status_ok:
			return true;
		
		case status_end_element_mismatch: {
			const char* expl = "Some HTML tags must be made explicitly self closing, such as '<img/>', '<meta/>' and others.";
			
			int row, col;
			if (findRowCol(in, result.offset, row, col) && row > 0)
				errorf(file, row, "%s. %s", result.description(), expl);
			else
				errorf(file, "%s. %s", result.description(), expl);
			
			return false;
		} break;
		
		case status_unrecognized_tag:
		case status_bad_pi:
		case status_bad_comment:
		case status_bad_cdata:
		case status_bad_doctype:
		case status_bad_pcdata:
		case status_bad_start_element:
		case status_bad_attribute:
		case status_bad_end_element:
		case status_no_document_element: {
			
			int row, col;
			if (findRowCol(in, result.offset, row, col) && row > 0){
				errorf(file, row, "%s.", result.description());
				return false;
			}
			
			goto default_err;
		} break;
		
		default:{
			default_err:
			errorf(file, "%s.", result.description());
			return false;
		} break;
		
	}
	
	return false;
}


// ------------------------------------------------------------------------------------------ //