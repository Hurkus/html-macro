#include "MacroParser.hpp"
#include <array>
#include <string_view>
#include <algorithm>

#include "DEBUG.hpp"

using namespace std;
using namespace pugi;


// ----------------------------------- [ Constants ] ---------------------------------------- //


static constexpr array<string_view,14> voidTags = {
	"area", "base", "br", "col", "embed", "hr", "img", "input",
	"link", "meta", "param", "source", "track", "wbr"
};


static constexpr array<string_view,2> noEscapeTags = {
	"script", "style"
};


static constexpr string_view tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";


// ----------------------------------- [ Structures ] --------------------------------------- //


struct DocWriter {
public:
	bool compress = false;
	ostream* out = nullptr;
	
public:
	void write(const xml_node node, int tab);
	
private:
	void writeElement(const xml_node element, int tab);
	void writeText(const xml_node text);
	void writeDocument(const xml_node root, int tab);
	void writeUnknown(const xml_node unknown, int tab);
	
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


template <typename C, typename E>
inline bool contains(const C& collection, const E& element){
	return find(collection.begin(), collection.end(), element) != collection.end();
}


constexpr string_view _tab(int tab) noexcept {
	assert(tab >= 0);
	if (tab >= int(tabs.length()))
		return tabs;
	return tabs.substr(0, tab);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void DocWriter::writeUnknown(const xml_node node, int tab){
	node.print(*out, "\t", pugi::format_raw, xml_encoding::encoding_utf8, tab);
}


void DocWriter::writeDocument(const xml_node root, int tab){
	for (const xml_node child : root.children()){
		write(child, tab);
		*out << '\n' << _tab(tab);
	}
}


void DocWriter::writeText(const xml_node txt){
	if (contains(noEscapeTags, txt.parent().name()))
		*out << txt.text().get();
	else
		txt.print(*out, "", pugi::format_default, xml_encoding::encoding_utf8);
}


void DocWriter::writeElement(const xml_node element, int tab){
	assert(!element.empty());
	assert(this->out != nullptr);
	
	// Write tag header
	*out << '<' << element.name();
	for (xml_attribute attr : element.attributes()){
		*out << ' ' << attr.name() << "=\"" << attr.value() << '"';
	}
	
	// Check self closing
	if (contains(voidTags, element.name())){
		*out << "/>";
		return;
	} else {
		*out << '>';
	}
	
	// Write children
	bool prev_txt = false;
	for (const xml_node child : element){
		
		if (child.type() == xml_node_type::node_pcdata){
			writeText(child);
			prev_txt = true;
		} else if (prev_txt){
			prev_txt = false;
			write(child, tab+1);
		} else {
			*out << '\n' << _tab(tab+1);
			write(child, tab+1);
		}
		
	}
	
	if (!prev_txt && !element.first_child().empty()){
		*out << '\n' << _tab(tab);
	}
	
	// Write end tag
	*out << "</" << element.name() << '>';
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void DocWriter::write(const xml_node root, int tab){
	assert(out != nullptr);
	
	switch (root.type()){
		case xml_node_type::node_element:
			writeElement(root, tab);
			break;
		
		case xml_node_type::node_document:
			writeDocument(root, tab);
			break;
		
		case xml_node_type::node_pi:
			assert(false && "No PI nodes should be present in the output.");
			break;
			
		case xml_node_type::node_null:
		case xml_node_type::node_declaration:
			break;
		
		default:
			writeUnknown(root, tab);
			break;
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void write(const xml_node root, ostream& out){
	DocWriter wr = {};
	wr.compress = false;
	wr.out = &out;
	wr.write(root, 0);
}


// ------------------------------------------------------------------------------------------ //