#pragma once
#include <string_view>


class Debugger {
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	virtual void error(std::string_view mark, const char* fmt, ...) const;
	virtual void warn(std::string_view mark, const char* fmt, ...) const;
	
public:
	virtual void source(const char*& file, std::string_view& buffer) const {}
	virtual std::string_view mark() const { return {}; }
	
// ------------------------------------------------------------------------------------------ //
};


class NoDebug : public Debugger {
	void error(std::string_view mark, const char* fmt, ...) const override {}
	void warn(std::string_view mark, const char* fmt, ...) const override {}
};


namespace html {
	struct Node;
}


class NodeDebugger : public Debugger {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	const html::Node& node;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	NodeDebugger(auto& node) : node{node} {}
	
public:
	void source(const char*& file, std::string_view& buffer) const override;
	std::string_view mark() const override;
	
// ------------------------------------------------------------------------------------------ //
};
