#include "Expr.hpp"

Expr Expr::Type::operator()() const{
	return Expr{*this};
}
Expr Expr::Type::operator()(const std::initializer_list<Expr>& il) const{
	return Expr{*this,il};
}

Expr::Expr(const Type& type):_type(&type){
	switch(_type->arity){
		case Type::NULLARY:
		case Type::INFINITARY:
			break;
		case Type::UNARY:
			_children = std::deque<Expr>({Expr()});
			break;
		case Type::BINARY:
			_children = std::deque<Expr>({Expr(),Expr()});
			break;
		case Type::TERNARY:
			_children = std::deque<Expr>({Expr(),Expr(),Expr()});
			break;
	}
}

Expr::Expr(const Type& type, const std::initializer_list<Expr>& il):_type(&type){
	switch(_type->arity){
		case Type::NULLARY:
			if(il.size()!=0)
				throw ExprError(Expr(type),"cannot give children to a nullary expr");
			break;
		case Type::INFINITARY:
			break;
		case Type::UNARY:
			if(il.size()!=1)
				throw ExprError(Expr(type),"cannot give "+std::to_string(il.size())+" children to a unary expr");
			break;
		case Type::BINARY:
			if(il.size()!=2)
				throw ExprError(Expr(type),"cannot give "+std::to_string(il.size())+" children to a binary expr");
			break;
		case Type::TERNARY:
			if(il.size()!=3)
				throw ExprError(Expr(type),"cannot give "+std::to_string(il.size())+" children to a ternary expr");
			break;
	}

	_children = std::deque<Expr>(il);
}

Expr& Expr::operator[](long n){
	if(n>=0 && n<_children.size())
		return _children[n];
	else
		throw ExprError(*this,"expr does not have a child at index "+std::to_string(n));
}
const Expr& Expr::operator[](long n) const{
	if(n>=0 && n<_children.size())
		return _children[n];
	else
		throw ExprError(*this,"expr does not have a child at index "+std::to_string(n));
}

void Expr::add_child(const Expr& expr){
	if(_type->arity!=Type::INFINITARY){
		throw ExprError(*this,"cannot add a new child to a non-infinitary expr");
	}
	_children.push_back(expr);
}

uint64_t Expr::hash() const {
	uint64_t h = type().id;
	h = (h<<19)^(h>>45);
	if(type()==Symbol||type()==Number){
		h^=_value;
	}
	else{
		for(const Expr& child : _children){
			h^=child.hash();
			h = (h<<19)^(h>>45);
		}
	}
	return h;
}

bool Expr::is_identical(const Expr& b) const {
	if(type()!=b.type())
		return false;
	if(child_count()!=b.child_count())
		return false;
	if(type()==Symbol||type()==Number)
		return _value==b._value;
	std::deque<Expr>::const_iterator a_iter = _children.begin();
	std::deque<Expr>::const_iterator b_iter = b._children.begin();
	while(a_iter!=_children.end()){
		if(!a_iter->is_identical(*b_iter))
			return false;
		a_iter++;
		b_iter++;
	}
	return true;
}
