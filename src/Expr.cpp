#include "Expr.hpp"

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
		throw ExprError(*this,string("an expr of type ")+type().name+" must have exactly "+std::to_string(type().arity)+" children");
	}
	_children.push_back(expr);
}

Expr::Iterator Expr::insert_child(const ConstIterator& pos, const Expr& expr){
	if(type().arity!=Type::INFINITARY){
		throw ExprError(*this,string("an expr of type ")+type().name+" must have exactly "+std::to_string(type().arity)+" children");
	}
	return Iterator(_children.insert(pos.iter,expr));
}

Expr::Iterator Expr::remove_child(const ConstIterator& pos){
	if(type().arity!=Type::INFINITARY){
		throw ExprError(*this,string("an expr of type ")+type().name+" must have exactly "+std::to_string(type().arity)+" children");
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
