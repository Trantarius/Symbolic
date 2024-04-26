#include "Expr.hpp"
#include <unordered_map>
#include <cmath>
#include <cfloat>

Expr::Expr(const Type& type):_type(&type){
	if(type.arity!=Type::INFINITARY){
		_children.resize(type.arity);
	}
}

Expr::Expr(const Type& type, const std::initializer_list<Expr>& il):_type(&type){
	if(type.arity!=Type::INFINITARY && il.size()!=type.arity)
		throw ExprError(Expr(type),"an expr of type "+type.name+" must have exactly "+std::to_string(type.arity)+" children");
	_children = std::deque<Expr>(il);
}

Expr& Expr::operator[](size_t n){
	if(n<_children.size())
		return _children[n];
	else
		throw ExprError(*this,"expr does not have a child at index "+std::to_string(n));
}
const Expr& Expr::operator[](size_t n) const{
	if(n<_children.size())
		return _children[n];
	else
		throw ExprError(*this,"expr does not have a child at index "+std::to_string(n));
}

void Expr::add_child(const Expr& expr){
	if(type().arity!=Type::INFINITARY){
		throw ExprError(*this,"an expr of type "+type().name+" must have exactly "+std::to_string(type().arity)+" children");
	}
	_children.push_back(expr);
}

hash_t Expr::hash() const {
	static_assert(sizeof(hash_t)==8,"Expr::hash algorithm assumes a 64-bit hash_t");
	static_assert(sizeof(hash_t)==sizeof(void*));
	static_assert(sizeof(hash_t)==sizeof(uni_value_t));
	hash_t h = reinterpret_cast<hash_t>(&type());
	h = (h<<19)^(h>>45);
	if(type().is_value_type()){
		h^=_value;
		h = (h<<19)^(h>>45);
	}
	for(const Expr& child : _children){
		h^=child.hash();
		h = (h<<19)^(h>>45);
	}
	return h;
}

bool Expr::is_identical_to(const Expr& b) const {
	if(type()!=b.type())
		return false;
	if(child_count()!=b.child_count())
		return false;
	if(type().is_value_type())
		return _value==b._value;
	std::deque<Expr>::const_iterator a_iter = _children.begin();
	std::deque<Expr>::const_iterator b_iter = b._children.begin();
	while(a_iter!=_children.end()){
		if(!a_iter->is_identical_to(*b_iter))
			return false;
		a_iter++;
		b_iter++;
	}
	return true;
}

std::deque<string> string_values;
std::unordered_map<string,int_value_t> string_value_ids;

template<> uni_value_t ValueExprType<string>::store(string v) {
	if(string_value_ids.contains(v)){
		return string_value_ids[v];
	}
	else{
		string_values.push_back(v);
		string_value_ids.emplace(v,string_values.size()-1);
		return string_values.size()-1;
	}
}
template<> string ValueExprType<string>::retrieve(uni_value_t v) {
	return string_values[v];
}

template<> uni_value_t ValueExprType<int_value_t>::store(int_value_t v) {
	static_assert(sizeof(uni_value_t)==sizeof(int_value_t));
	return *reinterpret_cast<uni_value_t*>(&v);
}
template<> int_value_t ValueExprType<int_value_t>::retrieve(uni_value_t v) {
	static_assert(sizeof(uni_value_t)==sizeof(int_value_t));
	return *reinterpret_cast<int_value_t*>(&v);
}

template<> uni_value_t ValueExprType<float_value_t>::store(float_value_t v){
	static_assert(sizeof(uni_value_t)==sizeof(float_value_t));
	return *reinterpret_cast<uni_value_t*>(&v);
}
template<> float_value_t ValueExprType<float_value_t>::retrieve(uni_value_t v) {
	static_assert(sizeof(uni_value_t)==sizeof(float_value_t));
	return *reinterpret_cast<float_value_t*>(&v);
}

template<> uni_value_t ValueExprType<bool_value_t>::store(bool_value_t v) {
	return static_cast<uni_value_t>(v);
}
template<> bool_value_t ValueExprType<bool_value_t>::retrieve(uni_value_t v) {
	return static_cast<bool_value_t>(v);
}


const Expr::Type& make_undefined(){
	static Expr::Type type;
	type.name="Undefined";
	type.print_string="∅";
	type.arity=Expr::Type::NULLARY;
	type.pemdas=-1000;
	Expr::Type::all_types.insert(&type);
	return type;
}
const Expr::Type& Undefined = make_undefined();
const Expr::Type& Expr::_Undefined = Undefined;

const InfinitaryExprType& make_list(){
	static InfinitaryExprType type;
	type.name="List";
	type.parse_string = R"(((?&EXPR)),((?&EXPR)))";
	type.print_string = "$1, $2";
	type.arity = Expr::Type::INFINITARY;
	type.pemdas = 1000;
	type.flags = Expr::Type::ASSOCIATIVE;
	Expr::Type::all_types.insert(&type);
	return type;
}
const InfinitaryExprType& List = make_list();

int_value_t add_int_calc(const Expr&,const std::deque<int_value_t>& args){
	int_value_t sum=0;
	for(int_value_t n : args){
		sum+=n;
	}
	return sum;
}

float_value_t add_float_calc(const Expr&, const std::deque<float_value_t>& args){
	float_value_t sum=0;
	for(float_value_t n : args){
		sum+=n;
	}
	return sum;
}

const InfinitaryExprType& make_add(){
	static InfinitaryExprType type;
	type.name="Add";
	type.parse_string = R"(((?&EXPR))\+((?&EXPR)))";
	type.print_string = "$1 + $2";
	type.arity = Expr::Type::INFINITARY;
	type.pemdas = 60;
	type.flags = Expr::Type::ASSOCIATIVE|
							 Expr::Type::COMMUTATIVE|Expr::Type::ENTRYWISE;
	type.f_int_calculate = add_int_calc;
	type.f_float_calculate = add_float_calc;
	Expr::Type::all_types.insert(&type);
	return type;
}
const InfinitaryExprType& Add = make_add();

int_value_t sub_int_calc(const Expr&, const std::deque<int_value_t>& args){
	return args[0]-args[1];
}

float_value_t sub_float_calc(const Expr&, const std::deque<float_value_t>& args){
	return args[0]-args[1];;
}

const BinaryExprType& make_sub(){
	static BinaryExprType type;
	type.name = "Sub";
	type.parse_string = R"(((?:(?&EXPR)\-)*(?&EXPR))(?<=[\w\)\]\}])\s*\-((?&EXPR)))";
	type.print_string = "$1 - $2";
	type.arity = Expr::Type::BINARY;
	type.pemdas = 50;
	type.flags = Expr::Type::ENTRYWISE;
	type.f_int_calculate = sub_int_calc;
	type.f_float_calculate = sub_float_calc;
	Expr::Type::all_types.insert(&type);
	return type;
}
const BinaryExprType& Sub = make_sub();

int_value_t mul_int_calc(const Expr&, const std::deque<int_value_t>& args){
	int_value_t prod=0;
	for(int_value_t n : args){
		prod*=n;
	}
	return prod;
}

float_value_t mul_float_calc(const Expr&, const std::deque<float_value_t>& args){
	float_value_t prod=0;
	for(float_value_t n : args){
		prod*=n;
	}
	return prod;
}

const InfinitaryExprType& make_mul(){
	static InfinitaryExprType type;
	type.name="Mul";
	type.parse_string = R"(((?&EXPR))\*((?&EXPR)))";
	type.print_string = "$1*$2";
	type.arity = Expr::Type::INFINITARY;
	type.pemdas = 40;
	type.flags = Expr::Type::ASSOCIATIVE|Expr::Type::COMMUTATIVE|
							 Expr::Type::ENTRYWISE;
	type.f_int_calculate = mul_int_calc;
	type.f_float_calculate = mul_float_calc;
	Expr::Type::all_types.insert(&type);
	return type;
}
const InfinitaryExprType& Mul = make_mul();

int_value_t div_int_calc(const Expr&, const std::deque<int_value_t>& args){
	return args[0]/args[1];
}

float_value_t div_float_calc(const Expr&, const std::deque<float_value_t>& args){
	return args[0]/args[1];
}

bool div_is_int_calc_defined(const Expr&, const std::deque<int_value_t>& args){
	return args[0] && !(args[0]%args[1]);
}

bool div_is_float_calc_defined(const Expr&, const std::deque<float_value_t>& args){
	return fabs(args[0])>DBL_EPSILON;
}

const BinaryExprType& make_div(){
	static BinaryExprType type;
	type.name = "Div";
	type.parse_string = R"(((?:(?&EXPR)/)*(?&EXPR))/((?&EXPR)))";
	type.print_string = "$1/$2";
	type.arity = Expr::Type::BINARY;
	type.pemdas = 30;
	type.flags = Expr::Type::ENTRYWISE;
	type.f_int_calculate = div_int_calc;
	type.f_float_calculate = div_float_calc;
	type.f_is_int_calc_defined = div_is_int_calc_defined;
	type.f_is_float_calc_defined = div_is_float_calc_defined;
	Expr::Type::all_types.insert(&type);
	return type;
}
const BinaryExprType& Div = make_div();

int_value_t neg_int_calc(const Expr&, const std::deque<int_value_t>& args){
	return -args[0];
}

float_value_t neg_float_calc(const Expr&, const std::deque<float_value_t>& args){
	return -args[0];
}

const UnaryExprType& make_neg(){
	static UnaryExprType type;
	type.name = "Neg";
	type.parse_string = R"(\s*-((?&EXPR)))";
	type.print_string = "-$1";
	type.arity = Expr::Type::UNARY;
	type.pemdas = 20;
	type.flags = Expr::Type::ENTRYWISE;
	type.f_int_calculate = neg_int_calc;
	type.f_float_calculate = neg_float_calc;
	Expr::Type::all_types.insert(&type);
	return type;
}
const UnaryExprType& Neg = make_neg();

int_value_t pow_int_calc(const Expr&, const std::deque<int_value_t>& args){
	switch(args[0]){
		case 0:
			return args[1]==0;
		case -1:
			return labs(args[1])%2 * 2 - 1;
		case 1:
			return 1;
		default:
			int_value_t i=1;
			int_value_t b=args[0];
			int_value_t p=args[1];
			while (p) {
				if (p & 1) {
					i *= b;
				}
				b *= b;
				p >>= 1;
			}
			return i;
	}
}

float_value_t pow_float_calc(const Expr&, const std::deque<float_value_t>& args){
	return pow(args[0],args[1]);
}

bool pow_is_int_calc_defined(const Expr&, const std::deque<int_value_t>& args){
	return args[1]>=0 || args[0]==1 || args[0]==0 || args[0]==-1;
}

bool pow_is_float_calc_defined(const Expr&, const std::deque<float_value_t>& args){
	return args[0]>=0;
}

const BinaryExprType& make_pow(){
	static BinaryExprType type;
	type.name = "Div";
	type.parse_string = R"(((?&EXPR))\^((?&EXPR)(?:\^(?&EXPR))*))";
	type.print_string = "$1^$2";
	type.arity = Expr::Type::BINARY;
	type.pemdas = 10;
	type.flags = Expr::Type::ENTRYWISE;
	type.f_int_calculate = pow_int_calc;
	type.f_float_calculate = pow_float_calc;
	type.f_is_int_calc_defined = pow_is_int_calc_defined;
	type.f_is_float_calc_defined = pow_is_float_calc_defined;
	Expr::Type::all_types.insert(&type);
	return type;
}
const BinaryExprType& Pow = make_pow();

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

const ValueExprType<string>& make_symbol(){
	static ValueExprType<string> type;
	type.name = "Symbol";
	type.parse_string = R"(\s*([a-zA-Z_]+)\s*)";
	type.arity = Expr::Type::NULLARY;
	type.pemdas = -10;
	type.flags = Expr::Type::VALUE_TYPE;
	type.f_parser = symbol_parser;
	type.f_printer = symbol_printer;
	Expr::Type::all_types.insert(&type);
	return type;
}
const ValueExprType<string>& Symbol = make_symbol();

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

int_value_t integer_int_calc(const Expr& ex, const std::deque<int_value_t>&){
	return Integer.value(ex);
}

float_value_t integer_float_calc(const Expr& ex, const std::deque<float_value_t>&){
	return Integer.value(ex);
}

const ValueExprType<int_value_t>& make_integer(){
	static ValueExprType<int_value_t> type;
	type.name = "Integer";
	type.parse_string = R"(\s*([0-9]+)\s*)";
	type.arity = Expr::Type::NULLARY;
	type.pemdas = -10;
	type.flags = Expr::Type::VALUE_TYPE|Expr::Type::CONSTANT;
	type.f_parser = integer_parser;
	type.f_printer = integer_printer;
	type.f_int_calculate = integer_int_calc;
	type.f_float_calculate = integer_float_calc;
	Expr::Type::all_types.insert(&type);
	return type;
}
const ValueExprType<int_value_t>& Integer = make_integer();
