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
