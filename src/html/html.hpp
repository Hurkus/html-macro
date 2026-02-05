#pragma once
#include <cassert>
#include <string>
#include <vector>
#include <memory>

#include "allocator.hpp"
#include "charalloc.hpp"
#include "EnumOperators.hpp"


namespace html {
	enum class NodeType : uint8_t;
	enum class NodeOptions : uint8_t;
	
	struct ParseResult;
	struct Node;
	struct Attr;
	class Document;
}



enum class html::NodeType : uint8_t {
	TAG,		// <tag ... >
	DIRECTIVE,	// <! ... >
	COMMENT,	// <!-- ... -->
	TEXT,		// <...>text</...>
	ROOT		// Internal for marking root.
};


enum class html::NodeOptions : uint8_t {
	NONE          = 0,
	OWNED_NAME    = 1 << 0,	// Name belongs to the Document str buffer; should be deleted on destruction.
	OWNED_VALUE   = 1 << 1,	// Value belongs to the Document str buffer; should be deleted on destruction.
	INTERPOLATE   = 1 << 2,	// Text content of `value` should be interpolated for expressions `{}`.
	SINGLE_QUOTE  = 1 << 3, // Attribute value is in single quotes.
	SELF_CLOSE    = 1 << 4, // <tag/>
	SPACE_BEFORE  = 1 << 5, // Node is prefixed (before opening tag) with whitespace.
	SPACE_AFTER   = 1 << 6, // Node is suffixed (after closing tag) with whitespace.
};
ENUM_OPERATORS(html::NodeOptions);



struct html::ParseResult {
// -------------------------------- //
public:
	enum class Status {
		OK,
		ERROR,					// Unknown error.
		MEMORY,					// Out of memory.
		UNEXPECTED_CHAR,
		UNCLOSED_TAG,			// ...>
		UNCLOSED_STRING,		// ..."
		UNCLOSED_COMMENT,		// ...-->
		INVALID_TAG_NAME,		// <...
		INVALID_TAG_CHAR,		// <...>
		MISSING_END_TAG,		// Some tags are unclosed: </tag>
		INVALID_END_TAG,		// Too many </tag>.
		MISSING_ATTR_VALUE		// attr=
	};
	
// -------------------------------- //
public:
	Status status;
	std::string_view mark;			// Substring that caused an error.
	std::vector<Node*> macros;		// Pointers to nodes `<MACRO>`
	
// -------------------------------- //
public:
	static const char* msg(Status status) noexcept;
	
	const char* msg() const noexcept {
		return msg(status);
	}
	
public:
	explicit operator bool() const {
		return status == Status::OK;
	}
	
// -------------------------------- //
};



struct html::Node {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	NodeType type = NodeType::TAG;
	NodeOptions options = NodeOptions::NONE;
	
	uint32_t value_len = 0;			// Length of `value_p`.
	const char* value_p = nullptr;	// Unterminated name/value string.
	
	Node* parent = nullptr;
	Node* child = nullptr;			// First/last child in linked list.
	Node* next = nullptr;			// Linked list.
	
	Attr* attribute = nullptr;		// First/last attribute in linked list.
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	// Get node value.
	std::string_view value() const {
		return std::string_view(value_p, value_len);
	}
	
	// Remove (maybe deallocate) value string.
	void value(Document& root, nullptr_t) noexcept;
	
	// Set value to const string, or something not allocated by the document's char allocator.
	// `str` will have to be deleted manualy after the node goes out of scope
	void value(Document& root, std::string_view str) noexcept;
	
	// Set value to owned string allocated by the document's char allocator.
	// `str` will be deleted after the node goes ouf of scope.
	void value(Document& root, char* str, size_t len) noexcept;
	
public:
	// Get node name
	std::string_view name() const {
		return this->value();
	}
	
	// Remove (maybe deallocate) name string.
	void name(Document& root, nullptr_t) noexcept {
		this->value(root, nullptr);
	}
	
	// Set name to const string, or something not allocated by the document's char allocator.
	// `str` will have to be deleted manualy after the node goes out of scope
	void name(Document& root, std::string_view str) noexcept {
		this->value(root, str);
	}
	
	// Set name to owned string allocated by the document's char allocator.
	// `str` will be deleted after the node goes ouf of scope.
	void name(Document& root, char* str, size_t len) noexcept {
		this->value(root, str, len);
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	// Extract from parent (can be null) and then deallocate.
	void remove(Document& root) noexcept;
	
public:
	Node* appendChild(Node* child) noexcept;
	Node* extractChild(Node* child) noexcept;
	void removeChildren(Document& root) noexcept;
	
public:
	Attr* appendAttribute(Attr*) noexcept;
	Attr* extractAttr(Attr*) noexcept;
	bool removeAttr(Document& root, std::string_view name) noexcept;
	void removeAttributes(Document& root) noexcept;
	
public:
	void clear(Document& root) noexcept {
		removeChildren(root);
		removeAttributes(root);
		value(root, nullptr);
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	Document& root(){
		if (parent != nullptr)
			return parent->root();
		assert(type == NodeType::ROOT);
		return *reinterpret_cast<Document*>(this);
	}
	
	const Document& root() const {
		if (parent != nullptr)
			return parent->root();
		assert(type == NodeType::ROOT);
		return *reinterpret_cast<const Document*>(this);
	}
	
// ------------------------------------------------------------------------------------------ //
};



struct html::Attr {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	NodeOptions options = NodeOptions::NONE;
	
	uint16_t name_len = 0;
	uint32_t value_len = 0;
	const char* name_p = nullptr;
	const char* value_p = nullptr;
	
	Attr* next = nullptr;	// Linked list.
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	// Get node value.
	std::string_view value() const {
		return std::string_view(value_p, value_len);
	}
	
	// Remove (maybe deallocate) value string.
	void value(Document& root, nullptr_t) noexcept;
	
	// Set value to const string, or something not allocated by the document's char allocator.
	// `str` will have to be deleted manualy after the node goes out of scope
	void value(Document& root, std::string_view str) noexcept;
	
	// Set value to owned string allocated by the document's char allocator.
	// `str` will be deleted after the node goes ouf of scope.
	void value(Document& root, char* str, size_t len) noexcept;
	
public:
	// Get node name
	std::string_view name() const {
		return std::string_view(name_p, name_len);
	}
	
	// Remove (maybe deallocate) name string.
	void name(Document& root, nullptr_t) noexcept;
	
	// Set name to const string, or something not allocated by the document's char allocator.
	// `str` will have to be deleted manualy after the node goes out of scope
	void name(Document& root, std::string_view str) noexcept;
	
	// Set name to owned string allocated by the document's char allocator.
	// `str` will be deleted after the node goes ouf of scope.
	void name(Document& root, char* str, size_t len) noexcept;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void clear(Document& root) noexcept {
		name(root, nullptr);
		value(root, nullptr);
	}
	
// ------------------------------------------------------------------------------------------ //
};



class html::Document : public html::Node {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	std::shared_ptr<const std::string> buffer;	// Source text.
	
public:
	std::shared_ptr<Allocator<Node>> nodeAlloc = std::make_shared<Allocator<Node>>();
	std::shared_ptr<Allocator<Attr>> attrAlloc = std::make_shared<Allocator<Attr>>();
	std::shared_ptr<CharAllocator> charAlloc = std::make_shared<CharAllocator>();
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	Document(){
		Node::type = NodeType::ROOT;
	}
	
	Document(Document&& o){
		swap(*this, o);
	}
	
	void operator=(Document&& o){
		swap(*this, o);
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Delete all data (child nodes, buffer, etc.).
	 *        All objects are considered to be either foreign owned,
	 *        or allocated with `nodeAlloc`, `attrAlloc` or `charAlloc`.
	 */
	void clear(){
		(Node&)(*this) = Node();
		Node::type = NodeType::ROOT;
		nodeAlloc = std::make_shared<Allocator<Node>>();
		attrAlloc = std::make_shared<Allocator<Attr>>();
		charAlloc = std::make_shared<CharAllocator>();
		buffer = {};
	}
	
public:
	/**
	 * @brief Parse html from string.
	 *        The document should first be `clear()` if it has been used before.
	 * @param buff String buffer. Must not change while `this` object is valid.
	 * @return `parse_result` containing parsing status.
	 */
	ParseResult parse(const std::shared_ptr<const std::string>& buff) noexcept;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	friend void swap(Document& a, Document& b) noexcept {
		std::swap((Node&)a, (Node&)b);
		std::swap(a.buffer, b.buffer);
		std::swap(a.nodeAlloc, b.nodeAlloc);
		std::swap(a.attrAlloc, b.attrAlloc);
		std::swap(a.charAlloc, b.charAlloc);
	}
	
// ------------------------------------------------------------------------------------------ //
};
