#pragma once
#include <string>

enum struct FuzzyBool:unsigned char{MAYBE,TRUE,FALSE};
using enum FuzzyBool;

constexpr FuzzyBool operator&&(FuzzyBool a, FuzzyBool b){
	if(a==FALSE||b==FALSE) return FALSE;
	if(a==MAYBE||b==MAYBE) return MAYBE;
	return TRUE;
}

constexpr FuzzyBool operator||(FuzzyBool a, FuzzyBool b){
	if(a==TRUE||b==TRUE) return TRUE;
	if(a==MAYBE||b==MAYBE) return MAYBE;
	return FALSE;
}

constexpr FuzzyBool operator!(FuzzyBool a){
	if(a==TRUE) return FALSE;
	if(a==FALSE) return TRUE;
	return MAYBE;
}

constexpr std::string to_string(FuzzyBool a){
	if(a==MAYBE) return "maybe";
	if(a==TRUE) return "true";
	return "false";
}
