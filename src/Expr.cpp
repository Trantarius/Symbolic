#include "Expr.hpp"

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

uint64_t Expr::hash() const {
	uint64_t h = reinterpret_cast<uint64_t>(&type());
	h = (h<<19)^(h>>45);
	if(type().uses_value()){
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
	if(type().uses_value())
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

const InfinitaryExprType& make_add(){
	static InfinitaryExprType type;
	type.name="Add";
	type.parse_string = R"(((?&EXPR))\+((?&EXPR)))";
	type.print_string = "$1 + $2";
	type.arity = Expr::Type::INFINITARY;
	type.pemdas = 60;
	type.flags = Expr::Type::OPERATOR|Expr::Type::ASSOCIATIVE|Expr::Type::COMMUTATIVE;
	Expr::Type::all_types.insert(&type);
	return type;
}
const InfinitaryExprType& Add = make_add();

const BinaryExprType& make_sub(){
	static BinaryExprType type;
	type.name = "Sub";
	type.parse_string = R"(((?:(?&EXPR)\-)*(?&EXPR))(?<=[\w\)\]\}])\s*\-((?&EXPR)))";
	type.print_string = "$1 - $2";
	type.arity = Expr::Type::BINARY;
	type.pemdas = 50;
	type.flags = Expr::Type::OPERATOR;
	Expr::Type::all_types.insert(&type);
	return type;
}
const BinaryExprType& Sub = make_sub();

const InfinitaryExprType& make_mul(){
	static InfinitaryExprType type;
	type.name="Mul";
	type.parse_string = R"(((?&EXPR))\*((?&EXPR)))";
	type.print_string = "$1*$2";
	type.arity = Expr::Type::INFINITARY;
	type.pemdas = 40;
	type.flags = Expr::Type::OPERATOR|Expr::Type::ASSOCIATIVE|Expr::Type::COMMUTATIVE;
	Expr::Type::all_types.insert(&type);
	return type;
}
const InfinitaryExprType& Mul = make_mul();

const BinaryExprType& make_div(){
	static BinaryExprType type;
	type.name = "Div";
	type.parse_string = R"(((?:(?&EXPR)/)*(?&EXPR))/((?&EXPR)))";
	type.print_string = "$1/$2";
	type.arity = Expr::Type::BINARY;
	type.pemdas = 30;
	type.flags = Expr::Type::OPERATOR;
	Expr::Type::all_types.insert(&type);
	return type;
}
const BinaryExprType& Div = make_div();

const UnaryExprType& make_neg(){
	static UnaryExprType type;
	type.name = "Neg";
	type.parse_string = R"(\s*-((?&EXPR)))";
	type.print_string = "-$1";
	type.arity = Expr::Type::UNARY;
	type.pemdas = 20;
	type.flags = Expr::Type::OPERATOR;
	Expr::Type::all_types.insert(&type);
	return type;
}
const UnaryExprType& Neg = make_neg();

const BinaryExprType& make_pow(){
	static BinaryExprType type;
	type.name = "Div";
	type.parse_string = R"(((?&EXPR))\^((?&EXPR)(?:\^(?&EXPR))*))";
	type.print_string = "$1^$2";
	type.arity = Expr::Type::BINARY;
	type.pemdas = 10;
	type.flags = Expr::Type::OPERATOR;
	Expr::Type::all_types.insert(&type);
	return type;
}
const BinaryExprType& Pow = make_pow();

const SymbolExprType& make_symbol(){
	static SymbolExprType type;
	type.name = "Symbol";
	type.parse_string = R"(\s*([a-zA-Z_]+)\s*)";
	type.arity = Expr::Type::NULLARY;
	type.pemdas = -10;
	type.flags = Expr::Type::USES_VALUE;
	Expr::Type::all_types.insert(&type);
	return type;
}
const SymbolExprType& Symbol = make_symbol();

const NumberExprType& make_number(){
	static NumberExprType type;
	type.name = "Number";
	type.parse_string = R"(\s*([0-9]+)\s*)";
	type.arity = Expr::Type::NULLARY;
	type.pemdas = -10;
	type.flags = Expr::Type::USES_VALUE|Expr::Type::CONSTANT;
	Expr::Type::all_types.insert(&type);
	return type;
}
const NumberExprType& Number = make_number();
