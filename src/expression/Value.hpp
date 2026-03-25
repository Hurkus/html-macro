#pragma once
#include <cassert>
#include <cstdint>
#include <vector>
#include "str_map.hpp"


struct Value {
// ----------------------------------- [ Structures ] --------------------------------------- //
public:
	enum class Type : uint32_t {
		NONE,
		LONG,
		DOUBLE,
		STRING,
		OBJECT		// Array and/or dictionary.
	};
	
	struct String;
	struct Object;
	
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	Type type = Type::NONE;
	
	union {
		long l = 0;
		double d;
		String* s;
		Object* o;
	} data;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	Value() = default;
	
	Value(nullptr_t) : type{Type::NONE} {}
	Value(long n) : type{Type::LONG}, data{.l = n} {}
	Value(double n) : type{Type::DOUBLE}, data{.d = n} {}
	Value(String* obj);
	Value(Object* obj);
	
	Value(std::string_view s);							// Create owned c-string.
	Value(std::string_view s1, std::string_view s2);	// Concat strings into owned c-string.
	
public:
	Value(const Value& o);
	
	Value(Value&& o){
		std::swap(this->type, o.type);
		std::swap(this->data, o.data);
	}
	
public:
	~Value();
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	bool toBool() const noexcept;
	std::string toStr() const;
	std::string& toStr(std::string& buff) const;
	
// ----------------------------------- [ Operators ] ---------------------------------------- //
public:
	bool equals(const Value& o) const noexcept;
	bool semanticEquals(const Value& o) const noexcept;
	bool semanticEquals(std::string_view s) const noexcept;

// ----------------------------------- [ Operators ] ---------------------------------------- //
public:
	bool operator==(const Value& o) const noexcept {
		return equals(o);
	}
	
	Value& operator=(const Value& o){
		if (this != &o){
			this->~Value();
			new (this) Value(o);
		}
		return *this;
	}
	
	Value& operator=(Value&& o){
		std::swap(this->type, o.type);
		std::swap(this->data, o.data);
		return *this;
	}

// ------------------------------------------------------------------------------------------ //	
};



struct Value::String {
// ---------------------------------------------------------------- //
public:
	int refs = 0;
	uint32_t len = 0;
	char str[];				// C-string
	
// ---------------------------------------------------------------- //
public:
	String() = default;
	String(Object&) = delete;
	String(Object&&) = delete;
	
	~String(){
		assert(refs == 0);
	}
	
// ---------------------------------------------------------------- //
public:
	String* ref(){
		refs++;
		return this;
	}
	
	void unref(){
		assert(refs > 0);
		if (--refs <= 0)
			delete this;
	}
	
public:
	std::string_view sv() const {
		return std::string_view(str, len);
	}
	
	char* begin(){
		return str;
	}
	
	char* end(){
		return str + len;
	}
	
public:
	static std::unique_ptr<String> create(uint32_t len);
	static std::unique_ptr<String> create(std::string_view str);
	static std::unique_ptr<String> create(std::string_view s1, std::string_view s2);
	
// ---------------------------------------------------------------- //
public:
	explicit operator std::string_view() const {
		return sv();
	}
	
// ---------------------------------------------------------------- //
};



struct Value::Object {
// ---------------------------------------------------------------- //
public:
	std::vector<Value> arr;
	str_map<Value> dict;
	int refs = 0;
	
// ---------------------------------------------------------------- //
public:
	Object() = default;
	Object(Object&) = delete;
	Object(Object&&) = delete;
	
	~Object(){
		assert(refs == 0);
	}
	
// ---------------------------------------------------------------- //
public:
	Object* ref(){
		refs++;
		return this;
	}
	
	void unref(){
		assert(refs > 0);
		if (--refs <= 0)
			delete this;
	}
	
	static std::unique_ptr<Object> create(){
		return std::make_unique<Object>();
	}
	
// ---------------------------------------------------------------- //
public:
	/**
	 * @brief Retrieve value of the underlying array.
	 * @return Pointer to array element or `null`.
	 */
	Value* get(long index){
		const size_t i = (index < 0) ? (arr.size() + index) : size_t(index);
		if (i < arr.size())
			return &arr[i];
		return nullptr;
	}
	
	/**
	 * @brief Retrieve value of the underlying array.
	 * @return Pointer to array element or `null`.
	 */
	const Value* get(long index) const {
		const size_t i = (index < 0) ? (arr.size() + index) : size_t(index);
		if (i < arr.size())
			return &arr[i];
		return nullptr;
	}
	
	/**
	 * @brief Retrieve entry value of the underlying dictionary.
	 * @param property Dictionary key.
	 * @return Pointer to dictionary element or `null`.
	 */
	Value* get(std::string_view property){
		return dict.get(property);
	}
	
	/**
	 * @brief Retrieve entry value of the underlying dictionary.
	 * @param property Dictionary key.
	 * @return Pointer to dictionary element or `null`.
	 */
	const Value* get(std::string_view property) const {
		return dict.get(property);
	}
	
	/**
	 * @brief Add key/value entry to the underlying dictionary.
	 * @param property Name of property.
	 * @param value Value of property.
	 */
	Value& insert(std::string_view property, Value&& value){
		return dict.insert(property, std::move(value));
	}
	
	/**
	 * @brief Add element to the underlying array.
	 * @param index Index at which to insert the value.
	 * @param value Element.
	 */
	Value& insert(size_t index, Value&& el){
		if (index >= arr.size())
			return arr.emplace_back(std::move(el));
		else
			return *arr.insert(arr.begin() + index, std::move(el));
	}
	
	/**
	 * @brief Remove entry from the underlying dictionary.
	 * @param property Dictionary entry key.
	 */
	void remove(std::string_view property){
		dict.remove(property);
	}
	
	/**
	 * @brief Remove element from the underlying array.
	 *        All subsequent elements are shifted to fill the hole in the array.
	 * @param index Index of the element to remove.
	 * @param count Amount of elements to remove after `index`.
	 */
	void remove(size_t index, size_t count = 1){
		if (index < arr.size()){
			size_t e = std::min(arr.size(), index + count);
			arr.erase(arr.begin() + index, arr.begin() + e);
		}
	}
	
	/**
	 * @brief Append all array elements of an obejct and insert all dictionary entries of the object `data.o`.
	 * @param Object The object from which to pull array elements and dictionary entries.
	 */
	void merge(Object&& obj);
	
// ---------------------------------------------------------------- //
public:
	bool operator==(const Object& o) const;
	
// ---------------------------------------------------------------- //
};
