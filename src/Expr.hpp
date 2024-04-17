#pragma once

#include <string>
#include <cstdint>

#include "refcount.hpp"

using std::string;

class Expr;

class Expr : public RefCounted{
public:
	enum Type{ADD,SUBTRACT,MULTIPLY,DIVIDE,NEGATE,EXPONENT,SYMBOL,NUMBER};
	inline static string type_to_string(Type type){
		switch(type){
			case ADD: return "ADD";
			case SUBTRACT: return "SUBTRACT";
			case MULTIPLY: return "MULTIPLY";
			case DIVIDE: return "DIVIDE";
			case NEGATE: return "NEGATE";
			case EXPONENT: return "EXPONENT";
			case SYMBOL: return "SYMBOL";
			case NUMBER: return "NUMBER";
		}
	}

	enum Arity{NULLARY,UNARY,BINARY,INFINITARY};
	inline static string arity_to_string(Arity arity){
		switch(arity){
			case NULLARY: return "NULLARY";
			case UNARY: return "UNARY";
			case BINARY: return "BINARY";
			case INFINITARY: return "INFINITARY";
		}
	}

public:
	virtual Type get_type() const = 0;
	virtual Arity get_arity() const = 0;
	virtual string to_string() const = 0;
	virtual uint64_t hash() const = 0;
	virtual bool is_identical(Ref<const Expr> expr) const = 0;

	class Iterator{
		Ref<Expr> owner;
		long child_idx = 0;
		Iterator(Ref<Expr> owner, long child_idx): owner(owner), child_idx(child_idx) {}
	public:
		Iterator()=default;
		Iterator(const Iterator&)=default;
		bool operator == (const Iterator& b) const { return owner==b.owner && child_idx==b.child_idx; }
		bool operator != (const Iterator& b) const { return !(*this==b); }
		Ref<Expr>& operator * () const ;
		Ref<Expr>& operator -> () const ;
		Iterator& operator ++ () { ++child_idx; return *this; }
		Iterator operator ++ (int) { return Iterator(owner,child_idx++); }
		Iterator& operator -- () { --child_idx; return *this; }
		Iterator operator -- (int) { return Iterator(owner,child_idx--); }
		friend class Expr;
	};

	class ConstIterator{
		Ref<const Expr> owner;
		long child_idx = 0;
		ConstIterator(Ref<const Expr> owner, long child_idx): owner(owner), child_idx(child_idx) {}
	public:
		ConstIterator()=default;
		ConstIterator(const ConstIterator&)=default;
		bool operator == (const ConstIterator& b) const { return owner==b.owner && child_idx==b.child_idx; }
		bool operator != (const ConstIterator& b) const { return !(*this==b); }
		const Ref<const Expr>& operator * () const ;
		const Ref<const Expr>& operator -> () const ;
		ConstIterator& operator ++ () { ++child_idx; return *this; }
		ConstIterator operator ++ (int) { return ConstIterator(owner,child_idx++); }
		ConstIterator& operator -- () { --child_idx; return *this; }
		ConstIterator operator -- (int) { return ConstIterator(owner,child_idx--); }
		friend class Expr;
	};

	Iterator begin() ;
	Iterator end() ;
	ConstIterator begin() const ;
	ConstIterator end() const ;

	static Ref<Expr> from_string(string str);
};

struct ExprSyntaxError : public std::runtime_error{};

struct NullaryExpr : public Expr{
	virtual Arity get_arity() const override{
		return NULLARY;
	}
};

struct UnaryExpr : public Expr{
	Ref<Expr> child;
	virtual Arity get_arity() const override{
		return UNARY;
	}
	virtual uint64_t hash() const override {
		uint64_t h = child->hash() ^ get_type();
		h = (h<<13) ^ (h>>51);
		return (h<<(get_type()%64)) ^ (h>>(64-get_type()%64));
	}
	virtual bool is_identical(Ref<const Expr> expr) const override{
		if(get_type()!=expr->get_type())
			return false;
		Ref<const UnaryExpr> uexpr = expr.cast_to<const UnaryExpr>();
		return child->is_identical(uexpr->child);
	}
};

struct BinaryExpr : public Expr{
	Ref<Expr> left,right;
	virtual Arity get_arity() const override{
		return BINARY;
	}
	virtual uint64_t hash() const override {
		uint64_t lh = left->hash();
		uint64_t rh = right->hash();
		uint64_t h = (lh>>13)^(lh<<51)^(rh<<13)^(rh>>51) ^ get_type();
		return (h<<(get_type()%64)) ^ (h>>(64-get_type()%64));
	}
	virtual bool is_identical(Ref<const Expr> expr) const override{
		if(get_type()!=expr->get_type())
			return false;
		Ref<const BinaryExpr> biexpr = expr.cast_to<const BinaryExpr>();
		return left->is_identical(biexpr->left) && right->is_identical(biexpr->right);
	}
};

struct InfinitaryExpr : public Expr{
	Ref<Array<Ref<Expr>>> children = new Array<Ref<Expr>>();
	virtual Arity get_arity() const override{
		return INFINITARY;
	}
	virtual uint64_t hash() const override {
		uint64_t h = get_type();
		for(Ref<Expr> child : *children){
			h = (h<<13)^(h>>51)^child->hash();
		}
		return (h<<(get_type()%64)) ^ (h>>(64-get_type()%64));
	}
	virtual bool is_identical(Ref<const Expr> expr) const override{
		if(get_type()!=expr->get_type())
			return false;
		Ref<const InfinitaryExpr> iexpr = expr.cast_to<const InfinitaryExpr>();
		if(children->size()!=iexpr->children->size())
			return false;
		Array<Ref<Expr>>::const_iterator a_child = children->begin();
		Array<Ref<Expr>>::const_iterator b_child = iexpr->children->begin();
		while(a_child!=children->end()){
			if(!(*a_child)->is_identical(*b_child))
				return false;
			a_child++;
			b_child++;
		}
		return true;
	}
};

struct Add : public InfinitaryExpr{
	virtual Type get_type() const override{ return ADD;}
	virtual string to_string() const override{
		string out="";
		for(Ref<Expr> child : children){
			out+=child->to_string()+" + ";
		}
		return out.substr(0,out.size()-3);
	}
};

struct Subtract : public BinaryExpr{
	virtual Type get_type() const override{ return SUBTRACT;}
	virtual string to_string() const override{
		string right_string;
		if(right->get_type()==ADD||right->get_type()==SUBTRACT)
			right_string="("+right->to_string()+")";
		else
			right_string=right->to_string();
		return left->to_string()+" - "+right_string;
	}
};

struct Multiply : public InfinitaryExpr{
	virtual Type get_type() const override{ return MULTIPLY;}
	virtual string to_string() const override{
		string out="";
		for(Ref<Expr> child : children){
			if(child->get_type()==ADD||child->get_type()==SUBTRACT)
				out+="("+child->to_string()+") * ";
			else
				out+=child->to_string()+" * ";
		}
		return out.substr(0,out.size()-3);
	}
};

struct Divide : public BinaryExpr{
virtual Type get_type() const override{ return DIVIDE;}
	virtual string to_string() const override{
		string left_string,right_string;
		if(left->get_type()==ADD||left->get_type()==SUBTRACT)
			left_string="("+left->to_string()+")";
		else
			left_string=left->to_string();
		if(right->get_type()==ADD||right->get_type()==SUBTRACT||right->get_type()==MULTIPLY||right->get_type()==DIVIDE)
			right_string="("+right->to_string()+")";
		else
			right_string=right->to_string();
		return left_string+" / "+right_string;
	}
};

struct Negate : public UnaryExpr{
virtual Type get_type() const override{ return NEGATE;}
	virtual string to_string() const override{
		string left_string,right_string;
		if(child->get_type()==ADD||child->get_type()==SUBTRACT||child->get_type()==EXPONENT)
			return "-("+child->to_string()+")";
		else
			return "-"+child->to_string();
	}
};

struct Exponent : public BinaryExpr{
virtual Type get_type() const override{ return EXPONENT;}
	virtual string to_string() const override{
		string left_string,right_string;
		if(left->get_type()==ADD||left->get_type()==SUBTRACT||left->get_type()==MULTIPLY||left->get_type()==DIVIDE||left->get_type()==EXPONENT)
			left_string="("+left->to_string()+")";
		else
			left_string=left->to_string();
		if(right->get_type()==ADD||right->get_type()==SUBTRACT||right->get_type()==MULTIPLY||right->get_type()==DIVIDE)
			right_string="("+right->to_string()+")";
		else
			right_string=right->to_string();
		return left_string+" ^ "+right_string;
	}
};

struct Symbol : public NullaryExpr{
	string name;
	virtual Type get_type() const override { return SYMBOL;}
	virtual string to_string() const override { return name;}
};

struct Number : public NullaryExpr{
	int64_t value;
	virtual Type get_type() const override { return NUMBER;}
	virtual string to_string() const override { return std::to_string(value);}
};
