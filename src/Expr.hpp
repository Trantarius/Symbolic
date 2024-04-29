#pragma once

#include <string>
#include <deque>

#include "error.hpp"
#include "Type.hpp"

using std::string;


class Expr {
	const Type* _type = &Undefined;
	std::deque<Expr> _children;
	uni_value_t _value=0;

	// for Type to directly initialize an Expr
	Expr(const Type* _t, std::deque<Expr>&& _c, uni_value_t _v):
		_type(_t), _children(std::move(_c)), _value(_v) {}

public:

	Expr()=default;
	Expr(const Expr&)=default;
	Expr(Expr&& ex):_type(ex._type),_children(std::move(ex._children)),_value(ex._value){
		ex._type=&Undefined;
	}
	Expr(const string& str);
	Expr(int_value_t);
	Expr(float_value_t);
	Expr(bool_value_t);

	const Type& type() const {return *_type; }
	bool is_identical_to(const Expr& expr) const;
	hash_t hash() const;

	Expr& operator=(const Expr&)=default;
	Expr& operator=(Expr&& b) {
		_type=b._type;
		_children=std::move(b._children);
		_value = b._value;
		return *this;
	};
	bool operator==(const Expr& expr) const{ return is_identical_to(expr); }

	class Iterator{
		std::deque<Expr>::iterator iter;
		Iterator(std::deque<Expr>::iterator it):iter(it){}
	public:
		Iterator()=default;
		Iterator(const Iterator&)=default;
		Iterator(Iterator&&)=default;
		Iterator& operator=(const Iterator&)=default;
		Iterator& operator=(Iterator&&)=default;
		bool operator == (const Iterator& b) const { return iter==b.iter; }
		bool operator != (const Iterator& b) const { return iter!=b.iter; }
		Expr& operator * () const { return *iter; }
		Expr* operator -> () const { return &*iter; }
		Iterator& operator ++ () { ++iter; return *this; }
		Iterator operator ++ (int) { return Iterator(iter++); }
		Iterator& operator -- () { --iter; return *this; }
		Iterator operator -- (int) { return Iterator(iter--); }
		friend class Expr;
		friend class ConstIterator;
	};

	class ConstIterator{
		std::deque<Expr>::const_iterator iter;
		ConstIterator(std::deque<Expr>::const_iterator it):iter(it){}
	public:
		ConstIterator()=default;
		ConstIterator(const ConstIterator&)=default;
		ConstIterator(ConstIterator&&)=default;
		ConstIterator& operator=(const ConstIterator&)=default;
		ConstIterator& operator=(ConstIterator&&)=default;
		ConstIterator(const Iterator& i):iter(i.iter){}
		ConstIterator(Iterator&& i):iter(std::move(i.iter)){}
		bool operator == (const ConstIterator& b) const { return iter==b.iter; }
		bool operator != (const ConstIterator& b) const { return iter!=b.iter; }
		const Expr& operator * () const { return *iter; }
		const Expr* operator -> () const { return &*iter; }
		ConstIterator& operator ++ () { ++iter; return *this; }
		ConstIterator operator ++ (int) { return ConstIterator(iter++); }
		ConstIterator& operator -- () { --iter; return *this; }
		ConstIterator operator -- (int) { return ConstIterator(iter--); }
		friend class Expr;
	};

	Iterator begin() { return Iterator(_children.begin()); }
	Iterator end() { return Iterator(_children.end()); }
	ConstIterator begin() const { return ConstIterator(_children.begin()); }
	ConstIterator end() const { return ConstIterator(_children.end()); }

	Expr& operator[](size_t n);
	const Expr& operator[](size_t n) const;
	size_t child_count() const { return _children.size(); }
	void add_child(const Expr& expr);
	Iterator insert_child(const ConstIterator& pos, const Expr& expr);
	Iterator remove_child(const ConstIterator& iter);

	friend class _ExprToFromStringImpl;
	friend class Type;
	template<typename T>
	friend class ValueType;

};

string to_string(const Expr& expr);



struct ExprError : public NamedError{
	Expr subject;
	ExprError(Expr subject, string what):
		NamedError("ExprError", what + "\nwith expression: "+to_string(subject)),subject(std::move(subject)){}
};

struct SyntaxError : public NamedError{
	string problem;
	string original;
	size_t position;
	SyntaxError(string problem, string original, size_t position):
		NamedError("SyntaxError", "'"+problem+"'\n"+original+"\n"+string(position,' ')+"^here"),
		problem(std::move(problem)), original(std::move(original)), position(position) {}
};

template<>
struct std::hash<Expr>{
	hash_t operator ()(const Expr& ex) const {
		return ex.hash();
	}
};

template<typename...Ts> requires (std::same_as<Expr,Ts>&&...)
Expr Type::operator()(Ts...args) const{
	if(arity!=INFINITARY)
		ASSERT_EQUAL(sizeof...(Ts),arity);
	return Expr(this, {std::move(args)...},0);
}
