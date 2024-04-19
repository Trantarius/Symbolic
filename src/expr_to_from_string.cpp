#include "Expr.hpp"

const std::regex empty_rex("\\s*");

Expr string_to_expr(string str, const std::vector<string>& bracketed);

bool string_to_expr_type(string str, const std::vector<string>& bracketed, const Expr::Type& type, Expr& out){
	std::smatch rex_result;

	switch(type.arity){
		case Expr::Type::NULLARY:
			if(std::regex_match(str,rex_result,type.parser)){
				if(type==Expr::Symbol){
					out=Expr::Symbol();
					if(Expr::symbol_ids.contains(rex_result[1])){
						out._value = Expr::symbol_ids[rex_result[1]];
					}else{
						out._value = Expr::symbol_names.size();
						Expr::symbol_names.push_back(rex_result[1]);
						Expr::symbol_ids.emplace(rex_result[1],out._value);
					}
				}
				else if(type==Expr::Number){
					out=Expr::Number();
					out._value = std::stol(rex_result[1]);
				}
				else{
					out=type();
				}
				return true;
			}
			else{
				return false;
			}
		case Expr::Type::UNARY:
			if(std::regex_match(str,rex_result,type.parser)){
				if(std::regex_match(rex_result[1].str(),empty_rex))
					throw MissingOperand(type.name);
				out=type({string_to_expr(rex_result[1],bracketed)});
				return true;
			}
			else{
				return false;
			}
		case Expr::Type::BINARY:
			if(type==Expr::Sub){
				// Sub shares an operator with Neg, so it needs special treatment
				std::sregex_iterator iter(str.begin(),str.end(),type.parser);
				while(iter!=std::sregex_iterator()){
					if(std::regex_match((*iter)[1].str(),empty_rex)){
						iter++;
						continue;
					}
					Expr left;
					try{
						left = string_to_expr((*iter)[1],bracketed);
					}catch(MissingRightOperand){
						iter++;
						continue;
					}
					if(std::regex_match((*iter)[2].str(),empty_rex))
						throw MissingRightOperand(type.name);
					out = Expr::Sub({left,string_to_expr((*iter)[2],bracketed)});
					return true;
				}
				return false;
			}
			else if(std::regex_match(str,rex_result,type.parser)){
				if(std::regex_match(rex_result[1].str(),empty_rex))
					throw MissingLeftOperand(type.name);
				if(std::regex_match(rex_result[2].str(),empty_rex))
					throw MissingRightOperand(type.name);
				out=type({string_to_expr(rex_result[1],bracketed),
									string_to_expr(rex_result[2],bracketed)});
				return true;
			}
			else{
				return false;
			}
		case Expr::Type::TERNARY:
			if(std::regex_match(str,rex_result,type.parser)){
				if(std::regex_match(rex_result[1].str(),empty_rex))
					throw MissingLeftOperand(type.name);
				if(std::regex_match(rex_result[2].str(),empty_rex))
					throw MissingCenterOperand(type.name);
				if(std::regex_match(rex_result[3].str(),empty_rex))
					throw MissingRightOperand(type.name);
				out=type({string_to_expr(rex_result[1],bracketed),
									string_to_expr(rex_result[2],bracketed),
									string_to_expr(rex_result[3],bracketed)});
				return true;
			}
			else{
				return false;
			}
		case Expr::Type::INFINITARY:
			if(std::regex_match(str,rex_result,type.parser)){
				if(std::regex_match(rex_result[1].str(),empty_rex))
					throw MissingLeftOperand(type.name);
				out=type();
				out.add_child(string_to_expr(rex_result[1],bracketed));
				str = rex_result[2];
				while(std::regex_match(str,rex_result,type.parser)){
					if(std::regex_match(rex_result[1].str(),empty_rex))
						throw MissingCenterOperand(type.name);
					out.add_child(string_to_expr(rex_result[1],bracketed));
					str = rex_result[2];
				}
				if(std::regex_match(str,empty_rex))
					throw MissingRightOperand(type.name);
				out.add_child(string_to_expr(str,bracketed));
				return true;
			}
			else{
				return false;
			}
	}
	return false;//unreachable, but prevents a warning
}

bool bracketed_to_expr(string str, const std::vector<string>& bracketed, Expr& out){
	static const std::regex bracket_escape_rex("\\s*\\x1F(\\d+)\\x1F\\s*");
	std::smatch rex_result;
	bool was_a_match = std::regex_match(str,rex_result,bracket_escape_rex);
	if(!was_a_match)
		return false;
	long idx = std::stol(rex_result[1]);
	string contents = bracketed[idx].substr(1,bracketed[idx].size()-2);
	char bracket = bracketed[idx][0];
	switch(bracket){
		case '(':
			out =  string_to_expr(contents,bracketed);
			return true;
		case '[':
			throw SyntaxError("square brackets [] not supported");
		case '{':
			throw SyntaxError("curly brackets {} not supported");
		default:
			return false;
	}
}

Expr string_to_expr(string str, const std::vector<string>& bracketed){
	Expr ret;

	for(const Expr::Type* type : Expr::Type::all_known_types){
		if(string_to_expr_type(str,bracketed,*type,ret))
			return ret;
	}

	if(bracketed_to_expr(str,bracketed,ret))
		return ret;

	throw SyntaxError("cannot parse \""+str+"\"");
}

std::vector<string> escape_brackets(string& str){
	static const std::regex bracketed_rex (R"((?:\([^\(\)\[\]\{\}]*\))|(?:\[[^\(\)\[\]\{\}]*\])|(?:\{[^\(\)\[\]\{\}]*\}))");
	std::vector<string> bracketed;
	std::smatch rex_result;
	while(std::regex_search(str,rex_result,bracketed_rex)){
		bracketed.push_back(rex_result.str());
		str.replace(rex_result.position(),rex_result.length(), "\x1F"+std::to_string(bracketed.size()-1)+"\x1F");
	}

	static const std::regex contains_bracket_rex("(.*)([\\(\\)\\[\\]\\{\\}])(.*)");
	if(std::regex_match(str,rex_result,contains_bracket_rex))
		throw SyntaxError("unbalanced bracket: "+rex_result[1].str());

	return bracketed;
}

Expr::Expr(string str){
	std::vector<string> bracketed = escape_brackets(str);
	*this=string_to_expr(str,bracketed);
}

string infinitary_to_string(const Expr& expr){
	std::vector<string> child_strings;
	for(const Expr& child : expr){
		if(child.type().pemdas>=0 && child.type().pemdas>=expr.type().pemdas)
			child_strings.push_back("("+to_string(child)+")");
		else
			child_strings.push_back(to_string(child));
	}
	if(child_strings.size()<2){
		throw ExprError(expr,"cannot to_string an infinitary expr with less than 2 children");
	}

	static const std::regex replacer_rex("\\x1F([^\\x1F\\x1E]*)\\x1F([^\\x1F\\x1E]*)\\x1E");

	while(child_strings.size()>=2){
		string target = "\x1E";
		target = "\x1F" + child_strings.back() + target;
		child_strings.pop_back();
		target = "\x1F" + child_strings.back() + target;
		child_strings.pop_back();
		string repl = std::regex_replace(target,replacer_rex,expr.type().printer);
		child_strings.push_back(repl);
	}

	return child_strings[0];
}

string to_string(const Expr& expr){
	string target;
	switch(expr.type().arity){
		case Expr::Type::NULLARY:
			if(expr.type()==Expr::Symbol){
				return Expr::symbol_names[expr._value];
			}
			else if(expr.type()==Expr::Number){
				return std::to_string(expr._value);
			}
			return expr.type().printer;
		case Expr::Type::INFINITARY:
			return infinitary_to_string(expr);
		case Expr::Type::TERNARY:
			if(expr[2].type().pemdas>=expr.type().pemdas && expr[2].type().pemdas>=0)
				target = "\x1F("+to_string(expr[2])+")";
			else
				target = "\x1F"+to_string(expr[2]);
		case Expr::Type::BINARY:
			if(expr[1].type().pemdas>=expr.type().pemdas && expr[1].type().pemdas>=0)
				target = "\x1F("+to_string(expr[1])+")" + target;
			else
				target = "\x1F"+to_string(expr[1]) + target;
		case Expr::Type::UNARY:
			if(expr[0].type().pemdas>=expr.type().pemdas && expr[0].type().pemdas>=0)
				target = "\x1F("+to_string(expr[0])+")" + target;
			else
				target = "\x1F"+to_string(expr[0]) + target;
			target += "\x1E";
	}

	static const std::regex replacer_rex("\\x1F([^\\x1F\\x1E]*)(?:\\x1F([^\\x1F\\x1E]*))?(?:\\x1F([^\\x1F\\x1E]*))?\\x1E");
	return std::regex_replace(target,replacer_rex,expr.type().printer);
}
