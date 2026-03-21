#pragma once
#include <cassert>
#include <memory>
#include <string_view>
#include <unordered_map>


template<typename T>
class str_map {
// ----------------------------------- [ Structures ] --------------------------------------- //
public:
	using key_type = std::string_view;
	using value_type = T;
	
	struct entry {
		value_type value;
		const char key[];	// c-string
	};
	
	using mapped_type = std::unique_ptr<entry>;
	using map_type = std::unordered_map<key_type,mapped_type>;
	
	struct iterator;
	struct const_iterator;
	
// ------------------------------------[ Properties ] --------------------------------------- //
private:
	map_type map;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	template<typename ...ARG>
	value_type& insert(const key_type& key, ARG&& ...args){
		auto p = map.try_emplace(key);
		const key_type& rKey = p.first->first;
		mapped_type& rEntry = p.first->second;
		
		// Created, create key-value pair
		if (p.second){
			rEntry = makeEntry(key);
			const_cast<key_type&>(rKey) = key_type(rEntry->key, key.length());
			new (&rEntry->value) value_type(std::forward<ARG>(args)...);
		} else {
			rEntry->value = value_type(std::forward<ARG>(args)...);
		}
		
		assert(rEntry != nullptr);
		return rEntry->value;
	}
	
	value_type& operator[](const key_type& key){
		auto p = map.try_emplace(key);
		const key_type& rKey = p.first->first;
		mapped_type& rEntry = p.first->second;
		
		// Created, create key-value pair
		if (p.second){
			rEntry = makeEntry(key);
			const_cast<key_type&>(rKey) = key_type(rEntry->key, key.length());
			new (&rEntry->value) value_type();
		}
		
		assert(rEntry != nullptr);
		return rEntry->value;
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	bool empty() const noexcept {
		return map.empty();
	}
	
	size_t size() const noexcept {
		return map.size();
	}
	
	entry* find(const key_type& key){
		auto p = map.find(key);
		if (p != nullptr)
			return p->second.get();
		return nullptr;
	}
	
	const entry* find(const key_type& key) const {
		auto p = map.find(key);
		if (p != nullptr)
			return p->second.get();
		return nullptr;
	}
	
	value_type* get(const key_type& key){
		auto p = map.find(key);
		if (p != nullptr)
			return &p->second->value;
		return nullptr;
	}
	
	const value_type* get(const key_type& key) const {
		auto p = map.find(key);
		if (p != nullptr)
			return &p->second->value;
		return nullptr;
	}
	
public:
	bool remove(const key_type& key){
		return map.erase(key) != 0;
	}
	
public:
	void clear(){
		map.clear();
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void copy(str_map<T>& dst){
		for (const auto& p : map){
			assert(p.second != nullptr);
			dst.insert(p.first, p.second->value);
		}
	}
	
	iterator begin(){
		return iterator(map.begin());
	}
	
	const_iterator begin() const {
		return const_iterator(map.begin());
	}
	
	iterator end(){
		return iterator(map.end());
	}
	
	const_iterator end() const {
		return const_iterator(map.end());
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
private:
	mapped_type makeEntry(const key_type& key){
		const size_t size = sizeof(entry) + sizeof(char)*(key.length() + 1);
		entry* e = (entry*)::operator new(size, std::align_val_t(alignof(entry::value)));
		
		// Copy key
		char* key_buff = const_cast<char*>(e->key);
		std::copy(key.data(), key.data() + key.length(), key_buff);
		key_buff[key.length()] = 0;
		
		return std::unique_ptr<entry>(e);
	}
	
// ------------------------------------------------------------------------------------------ //
};



template<typename T>
struct str_map<T>::iterator : public str_map<T>::map_type::iterator {
// ---------------------------------------------------------------- //
public:
	using map_iterator = str_map<T>::map_type::iterator;
	
	struct Pair {
		std::string_view key;
		T& value;
	};
	
// ---------------------------------------------------------------- //
public:
	iterator() = default;
	iterator(const auto& p) : map_iterator(p) {}
	iterator(auto&& p) : map_iterator(std::move(p)) {}
	
// ---------------------------------------------------------------- //
public:
	T* operator->() const {
		auto entry = static_cast<const map_iterator&>(*this).operator->();
		return entry->second->value;
		
	}
	
	Pair operator*() const {
		auto entry = static_cast<const map_iterator&>(*this).operator->();
		return Pair {
			.key = entry->first,
			.value = entry->second->value
		};
	}
	
// ---------------------------------------------------------------- //
};



template<typename T>
struct str_map<T>::const_iterator : public str_map<T>::map_type::const_iterator {
// ---------------------------------------------------------------- //
public:
	using map_iterator = str_map<T>::map_type::const_iterator;
	
	struct Pair {
		std::string_view key;
		const T& value;
	};
	
// ---------------------------------------------------------------- //
public:
	const_iterator() = default;
	const_iterator(const auto& p) : str_map<T>::map_type::const_iterator(p) {}
	const_iterator(auto&& p) : str_map<T>::map_type::const_iterator(std::move(p)) {}
	
// ---------------------------------------------------------------- //
public:
	const T* operator->() const {
		auto entry = static_cast<const map_iterator&>(*this).operator->();
		return entry->second->value;
		
	}
	
	Pair operator*() const {
		auto entry = static_cast<const map_iterator&>(*this).operator->();
		return Pair {
			.key = entry->first,
			.value = entry->second->value
		};
	}
	
// ---------------------------------------------------------------- //
};
