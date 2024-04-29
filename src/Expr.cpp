#include "Expr.hpp"
#include <unordered_map>
#include <cmath>
#include <cfloat>

Expr Expr::Type::operator()() const {
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

Expr::Expr(int_value_t v){
	*this=Integer(v);
}
Expr::Expr(float_value_t v){
	*this=Float(v);
}
Expr::Expr(bool_value_t v){
	*this=Bool(v);
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

Expr::Iterator Expr::insert_child(const ConstIterator& pos, const Expr& expr){
	if(type().arity!=Type::INFINITARY){
		throw ExprError(*this,"an expr of type "+type().name+" must have exactly "+std::to_string(type().arity)+" children");
	}
	return Iterator(_children.insert(pos.iter,expr));
}

Expr::Iterator Expr::remove_child(const ConstIterator& pos){
	if(type().arity!=Type::INFINITARY){
		throw ExprError(*this,"an expr of type "+type().name+" must have exactly "+std::to_string(type().arity)+" children");
	}
	return Iterator(_children.erase(pos.iter));
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

template<typename RET,typename...ARGS>
using FuncPtr = RET (*)(ARGS...);
template<typename T, FuncPtr<T,const Expr&> Expr::Type::*F_GET>
Expr add_perform_t(const Expr& ex){
	T total = 0;
	Expr ret = Add();
	for(const Expr& child : ex){
		if(child.type().*F_GET==nullptr){
			ret.add_child(child);
		}
		else{
			total += (child.type().*F_GET)(child);
		}
	}
	if(ret.child_count()==0){
		return Expr(total);
	}
	else{
		ret.add_child(Expr(total));
		return ret;
	}
}

Expr add_perform(const Expr& ex, bool allow_approx){
	ASSERT_EQUAL(ex.type(),Add);
	if(allow_approx){
		return add_perform_t<float_value_t,&Expr::Type::f_get_float>(ex);
	}else{
		return add_perform_t<int_value_t,&Expr::Type::f_get_int>(ex);
	}
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
	type.f_perform = add_perform;
	Expr::Type::all_types.insert(&type);
	return type;
}
const InfinitaryExprType& Add = make_add();

template<typename T, FuncPtr<T,const Expr&> Expr::Type::*F_GET>
Expr sub_perform_t(const Expr& ex){
	if(ex[0].type().*F_GET==nullptr || ex[1].type().*F_GET==nullptr)
		return ex;
	T left = (ex[0].type().*F_GET)(ex[0]);
	T right = (ex[1].type().*F_GET)(ex[1]);
	return Expr(left-right);
}

Expr sub_perform(const Expr& ex, bool allow_approx){
	ASSERT_EQUAL(ex.type(),Sub);
	if(allow_approx){
		return sub_perform_t<float_value_t, &Expr::Type::f_get_float>(ex);
	}else{
		return sub_perform_t<int_value_t, &Expr::Type::f_get_int>(ex);
	}
}

const BinaryExprType& make_sub(){
	static BinaryExprType type;
	type.name = "Sub";
	type.parse_string = R"(((?:(?&EXPR)\-)*(?&EXPR))(?<=[\w\)\]\}])\s*\-((?&EXPR)))";
	type.print_string = "$1 - $2";
	type.arity = Expr::Type::BINARY;
	type.pemdas = 50;
	type.flags = Expr::Type::ENTRYWISE;
	type.f_perform = sub_perform;
	Expr::Type::all_types.insert(&type);
	return type;
}
const BinaryExprType& Sub = make_sub();

template<typename T, FuncPtr<T,const Expr&> Expr::Type::*F_GET>
Expr mul_perform_t(const Expr& ex){
	T total = 1;
	Expr ret = Mul();
	for(const Expr& child : ex){
		if(child.type().*F_GET==nullptr){
			ret.add_child(child);
		}
		else{
			total *= (child.type().*F_GET)(child);
		}
	}
	if(ret.child_count()==0){
		return Expr(total);
	}
	else{
		ret.add_child(Expr(total));
		return ret;
	}
}

Expr mul_perform(const Expr& ex, bool allow_approx){
	ASSERT_EQUAL(ex.type(),Mul);
	if(allow_approx){
		return mul_perform_t<float_value_t,&Expr::Type::f_get_float>(ex);
	}else{
		return mul_perform_t<int_value_t,&Expr::Type::f_get_int>(ex);
	}
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
	type.f_perform = mul_perform;
	Expr::Type::all_types.insert(&type);
	return type;
}
const InfinitaryExprType& Mul = make_mul();

Expr div_perform(const Expr& ex, bool allow_approx){
	ASSERT_EQUAL(ex.type(),Div);
	if(allow_approx){
		if(ex[0].type().f_get_float==nullptr || ex[1].type().f_get_float==nullptr)
			return ex;
		float_value_t left = (ex[0].type().f_get_float)(ex[0]);
		float_value_t right = (ex[1].type().f_get_float)(ex[1]);
		if(fabs(right)<DBL_EPSILON)
			return Undefined();
		return Expr(left/right);
	}else{
		if(ex[0].type().f_get_int==nullptr || ex[1].type().f_get_int==nullptr)
			return ex;
		int_value_t left = (ex[0].type().f_get_int)(ex[0]);
		int_value_t right = (ex[1].type().f_get_int)(ex[1]);
		if(right==0)
			return Undefined();
		if(left%right!=0)
			return ex;
		return Expr(left/right);
	}
}

const BinaryExprType& make_div(){
	static BinaryExprType type;
	type.name = "Div";
	type.parse_string = R"(((?:(?&EXPR)/)*(?&EXPR))/((?&EXPR)))";
	type.print_string = "$1/$2";
	type.arity = Expr::Type::BINARY;
	type.pemdas = 30;
	type.flags = Expr::Type::ENTRYWISE;
	type.f_perform = div_perform;
	Expr::Type::all_types.insert(&type);
	return type;
}
const BinaryExprType& Div = make_div();

Expr neg_perform(const Expr& ex, bool allow_approx){
	ASSERT_EQUAL(ex.type(),Neg);
	if(allow_approx){
		if(ex[0].type().f_get_float==nullptr)
			return ex;
		return -(ex[0].type().f_get_float)(ex[0]);
	}else{
		if(ex[0].type().f_get_int==nullptr)
			return ex;
		return -(ex[0].type().f_get_int)(ex[0]);
	}
}

const UnaryExprType& make_neg(){
	static UnaryExprType type;
	type.name = "Neg";
	type.parse_string = R"(\s*-((?&EXPR)))";
	type.print_string = "-$1";
	type.arity = Expr::Type::UNARY;
	type.pemdas = 20;
	type.flags = Expr::Type::ENTRYWISE;
	type.f_perform = neg_perform;
	Expr::Type::all_types.insert(&type);
	return type;
}
const UnaryExprType& Neg = make_neg();

int_value_t int_pow(int_value_t b,int_value_t p){
	switch(b){
		case 0:
			return p==0;
		case -1:
			return labs(p)%2 * 2 - 1;
		case 1:
			return 1;
		default:
			int_value_t i=1;
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

Expr pow_perform(const Expr& ex, bool allow_approx){
	ASSERT_EQUAL(ex.type(),Pow);
	if(allow_approx){
		if(ex[0].type().f_get_float==nullptr || ex[1].type().f_get_float==nullptr)
			return ex;
		float_value_t left = (ex[0].type().f_get_float)(ex[0]);
		float_value_t right = (ex[1].type().f_get_float)(ex[1]);
		if(left<0)
			return Undefined();
		return Expr(pow(left,right));
	}else{
		if(ex[0].type().f_get_int==nullptr || ex[1].type().f_get_int==nullptr)
			return ex;
		int_value_t left = (ex[0].type().f_get_int)(ex[0]);
		int_value_t right = (ex[1].type().f_get_int)(ex[1]);
		if(!(right>=0 || left==1 || left==0 || left==-1))
			return ex;
		return Expr(int_pow(left,right));
	}
}

const BinaryExprType& make_pow(){
	static BinaryExprType type;
	type.name = "Div";
	type.parse_string = R"(((?&EXPR))\^((?&EXPR)(?:\^(?&EXPR))*))";
	type.print_string = "$1^$2";
	type.arity = Expr::Type::BINARY;
	type.pemdas = 10;
	type.flags = Expr::Type::ENTRYWISE;
	type.f_perform = pow_perform;
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
	type.parse_string = R"(\s*\b[a-zA-Z_]+\b\s*)";
	type.arity = Expr::Type::NULLARY;
	type.pemdas = -10;
	type.flags = Expr::Type::VALUE_TYPE;
	type.f_parser = symbol_parser;
	type.f_printer = symbol_printer;
	Expr::Type::all_types.insert(&type);
	return type;
}
const ValueExprType<string>& Symbol = make_symbol();

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

const ValueExprType<float_value_t>& make_float(){
	static ValueExprType<float_value_t> type;
	type.name = "Float";
	type.parse_string = R"(\s*\b[0-9]+\.[0-9]+\b\s*)";
	type.arity = Expr::Type::NULLARY;
	type.pemdas = -9;
	type.flags = Expr::Type::VALUE_TYPE|Expr::Type::CONSTANT;
	type.f_parser = float_parser;
	type.f_printer = float_printer;
	type.f_get_float = float_get_float;
	Expr::Type::all_types.insert(&type);
	return type;
}
const ValueExprType<float_value_t>& Float = make_float();

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

const ValueExprType<int_value_t>& make_integer(){
	static ValueExprType<int_value_t> type;
	type.name = "Integer";
	type.parse_string = R"(\s*\b[0-9]+\b\s*)";
	type.arity = Expr::Type::NULLARY;
	type.pemdas = -10;
	type.flags = Expr::Type::VALUE_TYPE|Expr::Type::CONSTANT;
	type.f_parser = integer_parser;
	type.f_printer = integer_printer;
	type.f_get_int = integer_get_int;
	type.f_get_float = integer_get_float;
	Expr::Type::all_types.insert(&type);
	return type;
}
const ValueExprType<int_value_t>& Integer = make_integer();


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

const ValueExprType<bool_value_t>& make_bool(){
	static ValueExprType<bool_value_t> type;
	type.name = "Bool";
	type.parse_string = R"(\s*\b(?:true|false|maybe)\b\s*)";
	type.arity = Expr::Type::NULLARY;
	type.pemdas = -9;
	type.flags = Expr::Type::VALUE_TYPE|Expr::Type::CONSTANT;
	type.f_parser = bool_parser;
	type.f_printer = bool_printer;
	type.f_get_bool = bool_get_bool;
	Expr::Type::all_types.insert(&type);
	return type;
}
const ValueExprType<bool_value_t>& Bool = make_bool();
