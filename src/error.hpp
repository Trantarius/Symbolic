#pragma once
#include <stdexcept>
#include <string>
using std::string;
using std::to_string;

struct NamedError : public std::runtime_error{
	string name;
	NamedError(const string& name, const string& what):std::runtime_error(what),name(name){}
};

struct InternalError : public NamedError{
	InternalError(string file, int line, string what):
		NamedError("InternalError",file+" line "+to_string(line)+": "+what){}
};

#define ERROR(MESSAGE) throw InternalError(__FILE__, __LINE__, MESSAGE)

#define ASSERT(CONDITION) if(!(CONDITION)) ERROR(#CONDITION " is false")

#define ASSERT_EQUAL(LEFT,RIGHT) if( (LEFT) != (RIGHT) ) ERROR(to_string(LEFT)+" != "+to_string(RIGHT))
