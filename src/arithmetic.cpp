#include "Expr.hpp"
#include <cmath>
#include <cfloat>

template<typename RET,typename...ARGS>
using FuncPtr = RET (*)(ARGS...);
template<typename T, FuncPtr<T,const Expr&> Type::*F_GET>
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
		return add_perform_t<float_value_t,&Type::f_get_float>(ex);
	}else{
		return add_perform_t<int_value_t,&Type::f_get_int>(ex);
	}
}

consteval Type make_add(){
	Type type;
	type.name="Add";
	type.parse_string = R"(((?&EXPR))\+((?&EXPR)))";
	type.print_string = "$1 + $2";
	type.arity = Type::INFINITARY;
	type.pemdas = 60;
	type.flags = Type::ASSOCIATIVE|
							 Type::COMMUTATIVE|Type::ENTRYWISE;
	type.f_perform = add_perform;
	return type;
}
constexpr Type Add = make_add();
REGISTER_TYPE(Add);

template<typename T, FuncPtr<T,const Expr&> Type::*F_GET>
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
		return sub_perform_t<float_value_t, &Type::f_get_float>(ex);
	}else{
		return sub_perform_t<int_value_t, &Type::f_get_int>(ex);
	}
}

consteval Type make_sub(){
	Type type;
	type.name = "Sub";
	type.parse_string = R"(((?:(?&EXPR)\-)*(?&EXPR))(?<=[\w\)\]\}])\s*\-((?&EXPR)))";
	type.print_string = "$1 - $2";
	type.arity = Type::BINARY;
	type.pemdas = 50;
	type.flags = Type::ENTRYWISE;
	type.f_perform = sub_perform;
	return type;
}
constexpr Type Sub = make_sub();
REGISTER_TYPE(Sub);

template<typename T, FuncPtr<T,const Expr&> Type::*F_GET>
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
		return mul_perform_t<float_value_t,&Type::f_get_float>(ex);
	}else{
		return mul_perform_t<int_value_t,&Type::f_get_int>(ex);
	}
}

consteval Type make_mul(){
	Type type;
	type.name="Mul";
	type.parse_string = R"(((?&EXPR))\*((?&EXPR)))";
	type.print_string = "$1*$2";
	type.arity = Type::INFINITARY;
	type.pemdas = 40;
	type.flags = Type::ASSOCIATIVE|Type::COMMUTATIVE|
							 Type::ENTRYWISE;
	type.f_perform = mul_perform;
	return type;
}
constexpr Type Mul = make_mul();
REGISTER_TYPE(Mul);

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

consteval Type make_div(){
	Type type;
	type.name = "Div";
	type.parse_string = R"(((?:(?&EXPR)/)*(?&EXPR))/((?&EXPR)))";
	type.print_string = "$1/$2";
	type.arity = Type::BINARY;
	type.pemdas = 30;
	type.flags = Type::ENTRYWISE;
	type.f_perform = div_perform;
	return type;
}
constexpr Type Div = make_div();
REGISTER_TYPE(Div);

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

consteval Type make_neg(){
	Type type;
	type.name = "Neg";
	type.parse_string = R"(\s*-((?&EXPR)))";
	type.print_string = "-$1";
	type.arity = Type::UNARY;
	type.pemdas = 20;
	type.flags = Type::ENTRYWISE;
	type.f_perform = neg_perform;
	return type;
}
constexpr Type Neg = make_neg();
REGISTER_TYPE(Neg);

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

consteval Type make_pow(){
	Type type;
	type.name = "Div";
	type.parse_string = R"(((?&EXPR))\^((?&EXPR)(?:\^(?&EXPR))*))";
	type.print_string = "$1^$2";
	type.arity = Type::BINARY;
	type.pemdas = 10;
	type.flags = Type::ENTRYWISE;
	type.f_perform = pow_perform;
	return type;
}
constexpr Type Pow = make_pow();
REGISTER_TYPE(Pow);
