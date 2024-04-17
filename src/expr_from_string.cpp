#include "Expr.hpp"
#include <regex>
#include <vector>

template<typename T>
using vector=std::vector<T>;

struct BracketEscapedString{
	string str;
	vector<BracketEscapedString> escaped;
};

// recursively replaces anything enclosed in brackets ()[]{} with (n) where n is an index of the bracket section
BracketEscapedString bracket_escape(string str){
	BracketEscapedString ret;
	string escaped;

	char closer=0;
}

Ref<Expr> Expr::from_string(string str){

}
