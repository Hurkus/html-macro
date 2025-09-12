#include "Write.hpp"
#include "html.hpp"
#include <vector>

using namespace std;
using namespace html;


// ---------------------------------- [ Definitions ] --------------------------------------- //


#define TAB_4		"\t\t\t\t"
#define SPACE_4		"    "
#define SPACE_16	SPACE_4 SPACE_4 SPACE_4 SPACE_4


// ----------------------------------- [ Structures ] --------------------------------------- //


// struct flushstream {
// 	ostream& out;
	
// 	template<typename T>
// 	flushstream& operator<<(T&& s){
// 		out << forward<T>(s);
// 		out.flush();
// 		return *this;
// 	}
	
// };


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr string_view trim_whitespace(string_view s){
	const char* beg = s.begin();
	const char* end = beg + s.length();
	
	while (beg != end && isspace(beg[+0])) beg++;
	while (end != beg && isspace(end[-1])) end--;
	
	return string_view(beg, end);
}


constexpr string_view tabs(int tab){
	constexpr string_view TABS = TAB_4 TAB_4 TAB_4 TAB_4;
	return TABS.substr(0, tab);
	// constexpr string_view TABS = SPACE_16 SPACE_16 SPACE_16 SPACE_16;
	// return TABS.substr(0, tab*4);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void writeAttributes(ostream& out, const Node& node, vector<const Attr*>& stack){
	assert(stack.empty());
	
	for (const Attr* attr = node.attribute ; attr != nullptr ; attr = attr->next){
		stack.emplace_back(attr);
	}
	
	while (!stack.empty()){
		const Attr& a = *stack.back();
		stack.pop_back();
		
		out << ' ' << a.name();
		if (a.value_p != nullptr){
			out << "=\"" << a.value() << '"';
		}
		
	}
}


// --------------------------------- [ Main Function ] -------------------------------------- //


bool write(ostream& out, const Document& doc, WriteOptions options){
	vector<const Node*> node_stack = {};
	vector<const Attr*> attr_stack = {};
	node_stack.reserve(128);
	attr_stack.reserve(16);
	
	assert(!(doc.options % NodeOptions::LIST_FORWARDS));
	for (const Node* child = doc.child ; child != nullptr ; child = child->next){
		node_stack.emplace_back(child);
	}
	
	int depth = 0;
	bool add_space = false;
	bool skip_space = false;
	
	while (!node_stack.empty()){
		const Node* node = node_stack.back();
		
		switch (node->type){
			case NodeType::TEXT:
				goto text;
			case NodeType::TAG:
				goto tag;
			case NodeType::DIRECTIVE:
				goto directive;
			default:
				goto pop;
		}
		
		
		text: {
			out << node->value();
			skip_space = true;
			add_space = false;
			goto pop;
		}
		
		
		directive: {
			if (!skip_space && (add_space || node->options % NodeOptions::SPACE_BEFORE)){
				out << '\n';
				out << tabs(depth);
			}
			
			out << '<' << node->value() << '>';
			
			skip_space = false;
			add_space = node->options % NodeOptions::SPACE_AFTER;
			goto pop;
		}
		
		
		tag: {
			if (!skip_space && (add_space || node->options % NodeOptions::SPACE_BEFORE)){
				out << '\n';
				out << tabs(depth);
			}
			
			add_space = false;
			skip_space = false;
			
			// Tag name
			out << '<' << node->name();
			writeAttributes(out, *node, attr_stack);
			
			if (node->child == nullptr && node->options % NodeOptions::SELF_CLOSE){
				out << "/>";
				add_space = node->options % NodeOptions::SPACE_AFTER;
				goto pop;
			} else {
				out << '>';
			}
			
			// Enqueue children
			if (node->child != nullptr){
				
				assert(!(node->options % NodeOptions::LIST_FORWARDS));
				for (const Node* child = node->child ; child != nullptr ; child = child->next){
					node_stack.emplace_back(child);
				}
				
				depth++;
				continue;
			}
			
			// Empty
			else {
				out << "</" << node->name() << ">";
				add_space = node->options % NodeOptions::SPACE_AFTER;
				goto pop;
			}
			
			continue;
		}
		
		
		pop: {
			node_stack.pop_back();
			
			// Last of children
			while (!node_stack.empty() && node_stack.back() == node->parent){
				node = node_stack.back();
				node_stack.pop_back();
				
				depth--;
				if (!skip_space && add_space){
					out << '\n';
					out << tabs(depth);
				}
				
				out << "</" << node->name() << ">";
				
				skip_space = false;
				add_space = node->options % NodeOptions::SPACE_AFTER;
			}
			
			continue;
		}
		
		continue;
	}
	
	if (add_space){
		out << '\n';
	}
	
	return true;
}


// ------------------------------------------------------------------------------------------ //