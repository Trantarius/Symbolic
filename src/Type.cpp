#include "Type.hpp"
#include "Expr.hpp"

#include <unordered_map>

Expr Type::operator()() const {
	switch(arity){
		case NULLARY:
			return Expr(this,std::deque<Expr>{},0);
		case UNARY:
			return Expr(this,std::deque<Expr>{Expr()},0);
		case BINARY:
			return Expr(this,std::deque<Expr>{Expr(),Expr()},0);
		case TERNARY:
			return Expr(this,std::deque<Expr>{Expr(),Expr(),Expr()},0);
		case INFINITARY:
			return Expr(this,std::deque<Expr>{},0);
		default:
			ERROR("should be unreachable");
	}
}

std::deque<string> string_values;
std::unordered_map<string,int_value_t> string_value_ids;

template<> Expr ValueType<string>::operator()(string v) const {
	uni_value_t uval;
	if(string_value_ids.contains(v)){
		uval = string_value_ids[v];
	}
	else{
		string_values.push_back(v);
		string_value_ids.emplace(v,string_values.size()-1);
		uval = string_values.size()-1;
	}
	return Expr(this,{},uval);
}
template<> string ValueType<string>::value(const Expr& ex) const{
	ASSERT_EQUAL(ex.type(),*this);
	return string_values[ex._value];
}

template<> Expr ValueType<int_value_t>::operator()(int_value_t v) const {
	static_assert(sizeof(uni_value_t)==sizeof(int_value_t));
	uni_value_t uval = *reinterpret_cast<uni_value_t*>(&v);
	return Expr(this,{},uval);
}
template<> int_value_t ValueType<int_value_t>::value(const Expr& ex) const{
	ASSERT_EQUAL(ex.type(),*this);
	static_assert(sizeof(uni_value_t)==sizeof(int_value_t));
	return *reinterpret_cast<const int_value_t*>(&(ex._value));
}

template<> Expr ValueType<float_value_t>::operator()(float_value_t v) const {
	static_assert(sizeof(uni_value_t)==sizeof(float_value_t));
	uni_value_t uval = *reinterpret_cast<uni_value_t*>(&v);
	return Expr(this,{},uval);
}
template<> float_value_t ValueType<float_value_t>::value(const Expr& ex) const{
	ASSERT_EQUAL(ex.type(),*this);
	static_assert(sizeof(uni_value_t)==sizeof(float_value_t));
	return *reinterpret_cast<const float_value_t*>(&(ex._value));
}

template<> Expr ValueType<bool_value_t>::operator()(bool_value_t v) const {
	uni_value_t uval = static_cast<uni_value_t>(v);
	return Expr(this,{},uval);
}
template<> bool_value_t ValueType<bool_value_t>::value(const Expr& ex) const{
	ASSERT_EQUAL(ex.type(),*this);
	return static_cast<bool_value_t>(ex._value);
}

consteval Type make_undefined(){
	Type type;
	type.name="Undefined";
	type.print_string="∅";
	type.arity=Type::NULLARY;
	type.pemdas=-1000;
	return type;
}
constexpr Type Undefined = make_undefined();
REGISTER_TYPE(Undefined);

consteval Type make_list(){
	Type type;
	type.name="List";
	type.parse_string = R"(((?&EXPR)),((?&EXPR)))";
	type.print_string = "$1, $2";
	type.arity = Type::INFINITARY;
	type.pemdas = 1000;
	type.flags = Type::ASSOCIATIVE;
	return type;
}
constexpr Type List = make_list();
REGISTER_TYPE(List);

Expr symbol_parser(const string& str){
	string trimmed;
	for(char c : str){
		if(c!=' ')
			trimmed.push_back(c);
	}
	return Symbol(trimmed);
}

string symbol_printer(const Expr& ex){
	return Symbol.value(ex);
}

consteval ValueType<string> make_symbol(){
	ValueType<string> type;
	type.name = "Symbol";
	type.parse_string = R"(\s*\b[a-zA-Z_]+\b\s*)";
	type.arity = Type::NULLARY;
	type.pemdas = -10;
	type.flags = Type::VALUE_TYPE;
	type.f_parser = symbol_parser;
	type.f_printer = symbol_printer;
	return type;
}
constexpr ValueType<string> Symbol = make_symbol();
REGISTER_TYPE(Symbol);

Expr float_parser(const string& str){
	string trimmed;
	for(char c : str){
		if(c!=' ')
			trimmed.push_back(c);
	}
	static_assert(std::same_as<float_value_t,double>);
	return Float(std::stod(trimmed));
}

string float_printer(const Expr& ex){
	return std::to_string(Float.value(ex));
}

float_value_t float_get_float(const Expr& ex){
	return Float.value(ex);
}

consteval ValueType<float_value_t> make_float(){
	ValueType<float_value_t> type;
	type.name = "Float";
	type.parse_string = R"(\s*\b[0-9]+\.[0-9]+\b\s*)";
	type.arity = Type::NULLARY;
	type.pemdas = -9;
	type.flags = Type::VALUE_TYPE|Type::CONSTANT;
	type.f_parser = float_parser;
	type.f_printer = float_printer;
	type.f_get_float = float_get_float;
	return type;
}
constexpr ValueType<float_value_t> Float = make_float();
REGISTER_TYPE(Float);

Expr integer_parser(const string& str){
	string trimmed;
	for(char c : str){
		if(c!=' ')
			trimmed.push_back(c);
	}
	return Integer(std::stol(trimmed));
}

string integer_printer(const Expr& ex){
	return std::to_string(Integer.value(ex));
}

int_value_t integer_get_int(const Expr& ex){
	return Integer.value(ex);
}

float_value_t integer_get_float(const Expr& ex){
	return Integer.value(ex);
}

consteval ValueType<int_value_t> make_integer(){
	ValueType<int_value_t> type;
	type.name = "Integer";
	type.parse_string = R"(\s*\b[0-9]+\b\s*)";
	type.arity = Type::NULLARY;
	type.pemdas = -10;
	type.flags = Type::VALUE_TYPE|Type::CONSTANT;
	type.f_parser = integer_parser;
	type.f_printer = integer_printer;
	type.f_get_int = integer_get_int;
	type.f_get_float = integer_get_float;

	return type;
}
constexpr ValueType<int_value_t> Integer = make_integer();
REGISTER_TYPE(Integer);


Expr bool_parser(const string& str){
	string trimmed;
	for(char c : str){
		if(c!=' ')
			trimmed.push_back(c);
	}
	if(trimmed=="true"){
		return Bool(TRUE);
	}
	else if(trimmed=="false"){
		return Bool(FALSE);
	}
	else{
		return Bool(MAYBE);
	}
}

string bool_printer(const Expr& ex){
	if(Bool.value(ex)==TRUE){
		return "true";
	}
	else if(Bool.value(ex)==FALSE){
		return "false";
	}
	else{
		return "maybe";
	}
}

bool_value_t bool_get_bool(const Expr& ex){
	return Bool.value(ex);
}

consteval ValueType<bool_value_t> make_bool(){
	ValueType<bool_value_t> type;
	type.name = "Bool";
	type.parse_string = R"(\s*\b(?:true|false|maybe)\b\s*)";
	type.arity = Type::NULLARY;
	type.pemdas = -9;
	type.flags = Type::VALUE_TYPE|Type::CONSTANT;
	type.f_parser = bool_parser;
	type.f_printer = bool_printer;
	type.f_get_bool = bool_get_bool;

	return type;
}
constexpr ValueType<bool_value_t> Bool = make_bool();
REGISTER_TYPE(Bool);
