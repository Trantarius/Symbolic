#include "Expr.hpp"
#include <regex>
#include <vector>


const std::regex empty_rex("\\s*");
const std::regex add_rex("(.*?)+(.*)");
const std::regex subtract_rex("(.*?)-(.*)");
const std::regex multiply_rex("(.*?)\\*(.*)");
const std::regex divide_rex("(.*?)/(.*)");
const std::regex exponent_rex("(.*)\\^(.*)");
const std::regex negate_rex("\\s*-(.*)");
const std::regex bracketed_rex (R"((?:\([^\(\)\[\]\{\}]*\))|(?:\[[^\(\)\[\]\{\}]*\])|(?:\{[^\(\)\[\]\{\}]*\}))");
const std::regex bracket_escape_rex(R"(\s*\x1F(\d+)\x1F\s*)");
const std::regex symbol_rex("\\s*(\\w+)\\s*");
const std::regex number_rex("\\s*(\\d+)\\s*");

Ref<Expr> from_bracket_escaped(string str, const std::vector<string>& bracketed);

struct MissingOperand : public ExprSyntaxError{};
struct MissingLeftOperand : public MissingOperand{};
struct MissingRightOperand : public MissingOperand{};

template<typename SubExpr>
Ref<SubExpr> parse_infinitary_operator(string str, const std::vector<string>& bracketed, const std::regex& oper_rex){
	std::smatch rex_match;
	Ref<SubExpr> expr = new SubExpr();
	while(std::regex_match(str,rex_match,oper_rex)){
		if(std::regex_match(rex_match[1].str(),empty_rex))
			throw MissingLeftOperand();
		if(std::regex_match(rex_match[2].str(),empty_rex))
			throw MissingRightOperand();
		expr->children->push_back(from_bracket_escaped(rex_match[1],bracketed));
		str = rex_match[2];
	}
	if(expr->children->empty())
		return nullptr;
	expr->children->push_back(from_bracket_escaped(str,bracketed));
	return expr;
}

Ref<Subtract> parse_subtract_operator(string str,const std::vector<string>& bracketed){
	std::smatch rex_match;
	std::sregex_iterator iter(str.begin(),str.end(),subtract_rex);
	while(iter!=std::sregex_iterator()){
		if(std::regex_match((*iter)[1].str(),empty_rex)){
			iter++;
			continue;
		}
		Ref<Expr> left;
		try{
			left = from_bracket_escaped((*iter)[1],bracketed);
		}catch(MissingRightOperand){
			iter++;
			continue;
		}
		if(std::regex_match(rex_match[2].str(),empty_rex))
			throw MissingRightOperand();
		Ref<Subtract> expr = new Subtract();
		expr->left = left;
		expr->right = from_bracket_escaped((*iter)[2],bracketed);
		return expr;
	}
	return nullptr;
}

template<typename SubExpr>
Ref<SubExpr> parse_binary_operator(string str, const std::vector<string>& bracketed, const std::regex& oper_rex){
	std::smatch rex_match;
	if(std::regex_match(str,rex_match,oper_rex)){
		if(std::regex_match(rex_match[1].str(),empty_rex))
			throw MissingLeftOperand();
		if(std::regex_match(rex_match[2].str(),empty_rex))
			throw MissingRightOperand();
		Ref<SubExpr> expr = new SubExpr();
		expr->left = from_bracket_escaped(rex_match[1],bracketed);
		expr->right = from_bracket_escaped(rex_match[2],bracketed);
		return expr;
	}
	return nullptr;
}

template<typename SubExpr>
Ref<SubExpr> parse_unary_operator(string str, const std::vector<string>& bracketed, const std::regex& oper_rex){
	std::smatch rex_match;
	if(std::regex_match(str,rex_match,oper_rex)){
		if(std::regex_match(rex_match[1].str(),empty_rex))
			throw MissingOperand();
		Ref<SubExpr> expr = new SubExpr();
		expr->child = from_bracket_escaped(rex_match[1],bracketed);
		return expr;
	}
	return nullptr;
}

Ref<Expr> from_bracket_escaped(string str, const std::vector<string>& bracketed){
	if(std::regex_match(str,empty_rex)) return nullptr;

	Ref<Add> add = parse_infinitary_operator<Add>(str,bracketed,add_rex);
	if(add!=nullptr) return add.cast_to<Expr>();

	Ref<Subtract> subtract = parse_subtract_operator(str,bracketed);
	if(subtract!=nullptr) return subtract.cast_to<Expr>();

	Ref<Multiply> multiply = parse_infinitary_operator<Multiply>(str,bracketed,multiply_rex);
	if(multiply!=nullptr) return multiply.cast_to<Expr>();

	Ref<Divide> divide = parse_binary_operator<Divide>(str,bracketed,divide_rex);
	if(divide!=nullptr) return divide.cast_to<Expr>();

	Ref<Exponent> exponent = parse_binary_operator<Exponent>(str,bracketed,exponent_rex);
	if(exponent!=nullptr) return exponent.cast_to<Expr>();

	Ref<Negate> negate = parse_unary_operator<Negate>(str,bracketed,negate_rex);
	if(negate!=nullptr) return negate.cast_to<Expr>();
}

Ref<Expr> Expr::from_string(string str){
	std::vector<string> bracketed;
	std::smatch rex_match;
	while(std::regex_search(str,rex_match,bracketed_rex)){
		bracketed.push_back(rex_match.str());
		str.replace(rex_match.position(),rex_match.length(), "\x1F"+std::to_string(bracketed.size()-1)+"\x1F");
	}

	return from_bracket_escaped(str,bracketed);
}
