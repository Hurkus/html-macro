#include "Write.hpp"
#include <vector>
#include <cstring>

#include "html.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;


// ---------------------------------- [ Definitions ] --------------------------------------- //


#define P(s)		ANSI_PURPLE s ANSI_RESET

#define TAB_4		"\t\t\t\t"
#define SPACE_4		"    "
#define SPACE_16	SPACE_4 SPACE_4 SPACE_4 SPACE_4


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool isWhitespace(char c){
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}


constexpr string_view trim_nl_suffix(string_view s){
	const char* beg = s.begin();
	const char* end = s.end();
	
	while (beg != end){
		end--;
		if (!isWhitespace(*end))
			break;
		else if (*end == '\n')
			return string_view(beg, end);
	}
	
	return s;
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
	
	assert(stack.empty());
}


static void writeIndentedText(ostream& out, string_view text, const int depth){
	// Write first line, before indentation
	{
		size_t i = text.find('\n');
		
		if (i == string_view::npos){
			out << text;
			return;
		}
		
		i++;
		out.write(text.data(), i);
		text.remove_prefix(i);
	}
	
	const char* beg = text.begin();
	const char* end = text.end();
	
	// Get indentation
	int indent = 0;
	while (&beg[indent] != end){
		if (beg[indent] != '\t')
			break;
		indent++;
	}
	
	while (beg != end){
		// Remove leading indentation
		for (int i = indent ; i > 0 && beg != end ; i--){
			if (*beg != '\t')
				break;
			beg++;
		}
		
		out << tabs(depth);
		
		// Print untill newline
		const char* p = (const char*)memchr(beg, '\n', end - beg);
		if (p == nullptr){
			out.write(beg, end - beg);
			break;
		}
		
		p++;
		out.write(beg, p - beg);
		beg = p;
	}
	
	return;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool writeCompressedStyleElement(ostream& out, const Node& style){
	for (const Node* child = style.child ; child != nullptr ; child = child->next){
		if (child->type != NodeType::TEXT){
			out.flush();
			ERROR("Invalid child element type. Element " P("<style>") " can only have text child elements.");
			return false;
		}
		
		string_view css = child->value();
		if (css.empty()){
			continue;
		}
		
		compressCSS(out, css.begin(), css.end());
		
		// Preserve whitespace between text chunks
		if (child->next != nullptr){
			if (isWhitespace(css.back()) || (child->next->value_len > 0 && isWhitespace(child->next->value_p[0])))
				out << '\n';
		}
		
	}
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool writeUncompressedHTML(ostream& out, const Document& doc, WriteOptions options){
	vector<const Node*> node_stack = {};	// Child node list is reversed.
	vector<const Attr*> attr_stack = {};	// Attr list is reversed.
	node_stack.reserve(64);
	attr_stack.reserve(16);
	
	int depth = 0;
	bool add_space = false;
	bool skip_space = false;
	
	// Push children of root element (reverse list)
	assert(!(doc.options % NodeOptions::LIST_FORWARDS));
	for (const Node* child = doc.child ; child != nullptr ; child = child->next){
		node_stack.emplace_back(child);
	}
	
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
			// Raw unmodified text
			if (node->parent != nullptr && node->parent->name() == "pre"sv){
				out << node->value();
				skip_space = true;
				add_space = false;
			}
			
			// Stack text nodes
			else if (node_stack.size() >= 2 && node_stack.end()[-2]->type == NodeType::TEXT){
				writeIndentedText(out, node->value(), depth);
			}
			
			// Single text node
			else {
				string_view text = trim_nl_suffix(node->value());
				bool trimmed = (text.length() != size_t(node->value_len));
				trimmed |= (node->options % NodeOptions::SPACE_AFTER);
				
				// Prefix with whitespace
				if (node->options % NodeOptions::SPACE_BEFORE){ [[unlikely]]
					if (!text.empty() && !isWhitespace(text[0]))
						out << '\n' << tabs(depth);
				}
				
				writeIndentedText(out, text, depth);
				
				skip_space = !trimmed;
				add_space = trimmed;
			}
			
			goto pop;
		}
		
		
		directive: {
			if (!skip_space && (add_space || node->options % NodeOptions::SPACE_BEFORE)){
				out << '\n' << tabs(depth);
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
				
				// Directly compress CSS
				if (node->name() == "style"sv && options % WriteOptions::COMPRESS_CSS){
					if (!writeCompressedStyleElement(out, *node))
						return false;
					goto close;
				}
				
				for (const Node* child = node->child ; child != nullptr ; child = child->next){
					node_stack.emplace_back(child);
				}
				
				depth++;
				continue;
			}
			
			// Empty
			else { close:
				out << "</" << node->name() << ">";
				add_space = node->options % NodeOptions::SPACE_AFTER;
				goto pop;
			}
			
			continue;
		}
		
		
		pop: {
			node_stack.pop_back();
			
			// Check if node is last of the siblings
			while (!node_stack.empty() && node_stack.back() == node->parent){
				node = node_stack.back();
				node_stack.pop_back();
				
				depth--;
				if (!skip_space && add_space){
					out << '\n' << tabs(depth);
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


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool shouldPreserveWhitespace(string_view tag) noexcept {
	switch (tag.length()){
		case 1:
			return tag == "p" || tag == "a" || tag == "i" || tag == "b" || tag == "u";
		case 2:
			if (tag == "li" || tag == "td" || tag == "th")
				return  true;
			return tag[0] == 'h' && '0' <= tag[1] && tag[1] <= '9';
		case 4:
			return tag == "span" || tag == "code";
		default:
			return false;
	}
}


static void writeCompressedText(ostream& out, string_view txt){
	const char* end = txt.end();
	const char* s = txt.begin();
	
	while (s != end){
		const char* _s = s;
		
		// Truncate whitespace (preserve one space or newline)
		bool nl = false;
		while (s != end && isWhitespace(*s)){
			nl |= (*s == '\n');
			s++;
		}
		
		if (s != _s){
			if (nl)
				out << '\n';
			else
				out << ' ';
		}
		
		if (s == end){
			break;
		}
		
		// Write solid-space
		_s = s++;
		while (s != end && !isWhitespace(*s)){
			s++;
		}
		
		out.write(_s, s - _s);
	}
	
}


static bool writeCompressedHTML(ostream& out, const Document& doc, WriteOptions options){
	vector<const Node*> node_stack = {};	// Child node list is reversed.
	vector<const Attr*> attr_stack = {};	// Attr list is reversed.
	node_stack.reserve(64);
	attr_stack.reserve(16);
	
	const Node* parent = &doc;
	int preserveSpaceIdx = 0;
	
	// Push children of root element (reverse list)
	assert(!(doc.options % NodeOptions::LIST_FORWARDS));
	for (const Node* child = doc.child ; child != nullptr ; child = child->next){
		node_stack.emplace_back(child);
	}
	
	while (!node_stack.empty()){
		const Node* node = node_stack.back();
		node_stack.pop_back();
		
		// Close previous element child groups
		while (parent != node->parent){
			assert(parent != nullptr && parent != &doc);
			
			if (shouldPreserveWhitespace(parent->name())){
				preserveSpaceIdx--;
			}
			
			out << "</" << parent->name() << '>';
			parent = parent->parent;
		}
		
		switch (node->type){
			case NodeType::TEXT:
				goto text;
			case NodeType::TAG:
				goto tag;
			case NodeType::DIRECTIVE:
				goto directive;
			default:
				continue;
		}
		
		tag: {
			if (preserveSpaceIdx > 0 && node->options % NodeOptions::SPACE_BEFORE){
				out << ' ';
			}
			
			out << '<' << node->name();
			writeAttributes(out, *node, attr_stack);
			
			// Close tag or whole element
			if (node->child == nullptr){
				if (node->options % NodeOptions::SELF_CLOSE)
					out << "/>";
				else
					out << "></" << node->name() << '>';
				continue;
			} else {
				out << '>';
			}
			
			// Directly compress CSS
			if (node->name() == "style"sv && options % WriteOptions::COMPRESS_CSS){
				if (!writeCompressedStyleElement(out, *node))
					return false;
				out << "</" << node->name() << '>';
				continue;
			}
			
			// Enqueue children
			for (const Node* child = node->child ; child != nullptr ; child = child->next){
				node_stack.emplace_back(child);
			}
			
			if (shouldPreserveWhitespace(node->name())){
				preserveSpaceIdx++;
			}
			
			parent = node;
		} continue;
		
		
		text: {
			writeCompressedText(out, node->value());
		} continue;
		
		
		directive: {
			out << '<' << node->value() << ">\n";
		} continue;
		
	}
	
	// Write missing tail elements
	while (parent != &doc){
		assert(parent != nullptr);
		out << "</" << parent->name() << '>';
		parent = parent->parent;
	}
	
	return true;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


bool write(ostream& out, const Document& doc, WriteOptions options){
	#ifdef DEBUG
		// Automatic flush after each write.
		out << std::unitbuf;
	#endif
	
	if (options % WriteOptions::COMPRESS_HTML)
		writeCompressedHTML(out, doc, options);
	else
		writeUncompressedHTML(out, doc, options);
	
	return true;
}


// ------------------------------------------------------------------------------------------ //