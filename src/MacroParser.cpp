#include "MacroParser.hpp"
#include <fstream>

#include "Format.hpp"

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


// ----------------------------------- [ Functions ] ---------------------------------------- //


void parseMacro(const char* file, xml_document& doc){
	ifstream in = ifstream(file);
	if (!in){
		throw ParsingException(format("Failed to open file '%s'.", file));
	}
	
	xml_parse_result result = doc.load(in, pugi::parse_default | pugi::parse_doctype, pugi::encoding_utf8);
	switch (result.status){
		case status_ok:
			break;
		
		case status_end_element_mismatch: {
			int row, col;
			string s;
			if (findRowCol(in, result.offset, row, col))
				s = format("%s. ", result.description());
			s += "Some HTML tags must be made explicitly self closing, such as '<img/>', '<meta/>' and others.";
			throw ParsingException(file, row, col, move(s));
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
			if (findRowCol(in, result.offset, row, col))
				throw ParsingException(file, row, col, format("%s.", result.description()));
			goto default_err;
		} break;
		
		default:{
			default_err:
			throw ParsingException(file, format("%s.", result.description()));
		} break;
		
	}
	
}


// ------------------------------------------------------------------------------------------ //