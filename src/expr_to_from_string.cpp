#include "Expr.hpp"
#include <boost/regex.hpp>
#include <iostream>

struct _ExprToFromStringImpl{
	inline static const boost::regex empty_rex{"\\s*"};
	inline static const string rex_header=R"((?(DEFINE)
		(?'BAL'(?:\((?&BAL)\)|\[(?&BAL)\]|\{(?&BAL)\}|[^\(\)\[\]\{\}\n])*?)
		(?'EXPR'[[:blank:]]*(?=\S)(?&BAL)(?<=\S)[[:blank:]]*)
	))";
	inline static const int rex_header_def_count = 2;

	inline static const boost::regex parenthetical_rex = boost::regex(
		rex_header + R"(\(((?&BAL))\))"
	);

	static Expr parse_string(const string& str, const string& original, size_t position);
};

Expr _ExprToFromStringImpl::parse_string(const string& str, const string& original, size_t position){
	Expr ret;
	std::cout<<"Parsing '"<<str<<"'"<<std::endl;

	if(boost::regex_match(str,empty_rex))
		throw ExprError(Expr(),__FILE__ ": " + std::to_string(__LINE__));

	for(const Expr::Type* exprtype : Expr::Type::all_known_types){
		boost::regex rex = boost::regex(rex_header+exprtype->parser,boost::regex_constants::mod_x);
		std::cout<<"Trying '"<<exprtype->parser<<"'"<<std::endl;
		boost::smatch results;
		if(boost::regex_match(str,results,rex)){
			std::cout<<"Matched "<<exprtype->name<<std::endl;
			for(int n=0;n<results.size();n++){
				std::cout<<"Result "<<std::to_string(n)<<" is '"<<results.str(n)<<"'"<<std::endl;
			}
			ret._type=exprtype;
			if(exprtype->arity!=NULLARY){
				assert(results.size()==exprtype->arity+(rex_header_def_count+1UL) || exprtype->arity==INFINITARY);
				for(size_t n=rex_header_def_count+1;n<results.size();n++){
					std::cout<<"Dispatching "<<results.str(n)<<std::endl;
					Expr child = parse_string(results[n],original,results.position(n)+position);
					if(exprtype->arity==INFINITARY && child.type()==*exprtype){
						while(!child._children.empty()){
							ret._children.push_back(std::move(child._children.front()));
							child._children.pop_front();
						}
					}
					else{
						ret._children.push_back(std::move(child));
					}
				}
			}
			if(*exprtype==Expr::Symbol){
				if(Expr::symbol_ids.contains(results[rex_header_def_count+1])){
					ret._value=Expr::symbol_ids[results[rex_header_def_count+1]];
				}
				else{
					ret._value=Expr::symbol_names.size();
					Expr::symbol_names.push_back(results[rex_header_def_count+1]);
					Expr::symbol_ids.emplace(results.str(rex_header_def_count+1),ret._value);
				}
			}
			else if(*exprtype==Expr::Number){
				ret._value=std::stol(results[rex_header_def_count+1]);
			}
			return ret;
		}
	}

	boost::smatch result;
	if(boost::regex_match(str, result, parenthetical_rex)){
		return parse_string(result.str(rex_header_def_count+1),original,
												position+result.position(rex_header_def_count+1));
	}

	throw SyntaxError(str,original,position);
}

Expr::Expr(const string& str){
	*this = _ExprToFromStringImpl::parse_string(str,str,0);
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

	static const boost::regex replacer_rex("\\x1F([^\\x1F\\x1E]*)\\x1F([^\\x1F\\x1E]*)\\x1E");

	while(child_strings.size()>=2){
		string target = "\x1E";
		target = "\x1F" + child_strings.back() + target;
		child_strings.pop_back();
		target = "\x1F" + child_strings.back() + target;
		child_strings.pop_back();
		string repl = boost::regex_replace(target,replacer_rex,expr.type().printer);
		child_strings.push_back(repl);
	}

	return child_strings[0];
}

string to_string(const Expr& expr){
	string target;
	switch(expr.type().arity){
		case NULLARY:
			if(expr.type()==Expr::Symbol){
				return Expr::symbol_names[expr._value];
			}
			else if(expr.type()==Expr::Number){
				return std::to_string(expr._value);
			}
			return expr.type().printer;
		case INFINITARY:
			return infinitary_to_string(expr);
		case TERNARY:
			if(expr[2].type().pemdas>=expr.type().pemdas && expr[2].type().pemdas>=0)
				target = "\x1F("+to_string(expr[2])+")";
			else
				target = "\x1F"+to_string(expr[2]);
			__attribute__ ((fallthrough));
		case BINARY:
			if(expr[1].type().pemdas>=expr.type().pemdas && expr[1].type().pemdas>=0)
				target = "\x1F("+to_string(expr[1])+")" + target;
			else
				target = "\x1F"+to_string(expr[1]) + target;
			__attribute__ ((fallthrough));
		case UNARY:
			if(expr[0].type().pemdas>=expr.type().pemdas && expr[0].type().pemdas>=0)
				target = "\x1F("+to_string(expr[0])+")" + target;
			else
				target = "\x1F"+to_string(expr[0]) + target;
			target += "\x1E";
	}

	static const boost::regex replacer_rex("\\x1F([^\\x1F\\x1E]*)(?:\\x1F([^\\x1F\\x1E]*))?(?:\\x1F([^\\x1F\\x1E]*))?\\x1E");
	return boost::regex_replace(target,replacer_rex,expr.type().printer);
}

