#pragma once

#include <string>
#include <deque>
#include <unordered_map>
#include <set>
#include <cstdint>
#include <vector>
#include <stdexcept>

using std::string;

enum Arity:uint8_t{NULLARY=0,UNARY=1,BINARY=2,TERNARY=3,INFINITARY=UINT8_MAX};

struct Expr {


	uint64_t hash() const;

	class Type{
		inline static uint64_t last_id=0;
		uint64_t id=last_id++ * 313 + 727;
		struct cmp_types{
			bool operator()(const Type* a, const Type* b) const {
				return a->pemdas > b->pemdas;
			}
		};
	public:
		inline static std::multiset<const Type*,cmp_types> all_known_types;
		string name;

		// regex to match for parsing; ((?&EXPR)) is a special marker for any sub expression
		string parser;

		// regex replace format for printing
		string printer;

		// required number of children (exact); INFINITARY for any amount
		Arity arity;

		//enum Arity{NULLARY,UNARY,BINARY,TERNARY,INFINITARY};
		//Arity arity;

		// determines parsing order and if parentheses are needed for to_string
		// <0 disables parentheses
		int pemdas;

		Type()=delete;
		Type(const Type&)=delete;
		Type(string name, string parser, string printer, Arity arity, int pemdas): name(name), parser(parser), printer(printer), arity(arity), pemdas(pemdas){
				all_known_types.insert(this);
			};
		bool operator==(const Type& b) const { return id==b.id; }
		bool operator!=(const Type& b) const { return id!=b.id; }
		Expr operator()() const;
		Expr operator()(const std::initializer_list<Expr>& il) const;
		friend uint64_t Expr::hash() const;
	};

	inline static const Type Add{"Add", R"(((?:(?&EXPR)\+)*(?&EXPR))\+((?&EXPR)))", "$1+$2", INFINITARY, 60};
	inline static const Type Sub{"Sub",
		R"(((?:(?&EXPR)\-)*(?&EXPR))(?<=[\w\)\]\}])\s*\-((?&EXPR)))", "$1-$2", BINARY, 50};
	inline static const Type Mul{"Mul", R"(((?:(?&EXPR)\*)*(?&EXPR))\*((?&EXPR)))", "$1*$2", INFINITARY, 40};
	inline static const Type Div{"Div", R"(((?:(?&EXPR)/)*(?&EXPR))/((?&EXPR)))", "$1/$2", BINARY, 30};
	inline static const Type Neg{"Neg", "-((?&EXPR))", "-$1", UNARY, 20};
	inline static const Type Pow{"Pow", R"(((?&EXPR))\^((?&EXPR)(?:\^(?&EXPR))*))", "$1^$2", BINARY, 10};
	inline static const Type Symbol{"Symbol", R"(\s*([a-zA-Z_]+)\s*)", "", NULLARY, -10};
	inline static const Type Number{"Number", R"(\s*([0-9]+)\s*)", "", NULLARY, -10};
	inline static const Type Undefined{"Undefined", "", "∅", NULLARY, -1000};

private:
	inline static std::vector<string> symbol_names;
	inline static std::unordered_map<string,long> symbol_ids;
	const Type* _type = &Undefined;
	std::deque<Expr> _children;
	int64_t _value=0;
public:

	Expr()=default;
	Expr(const Expr&)=default;
	Expr(Expr&& ex):_type(ex._type),_children(std::move(ex._children)),_value(ex._value){
		ex._type=&Undefined;
	}
	Expr(const Type& type);
	Expr(const Type& type, const std::initializer_list<Expr>& il);
	Expr(const string& str);

	const Type& type() const {return *_type; }
	bool is_identical(const Expr& epxr) const;

	Expr& operator=(const Expr&)=default;
	Expr& operator=(Expr&& b) {
		_type=b._type;
		_children=std::move(b._children);
		_value = b._value;
		return *this;
	};

	Expr& operator[](size_t n);
	const Expr& operator[](size_t n) const;
	size_t child_count() const { return _children.size(); }
	void add_child(const Expr& expr);

	class Iterator{
		Expr* owner = nullptr;
		long child_idx = 0;
		Iterator(Expr* owner, long child_idx): owner(owner), child_idx(child_idx) {}
	public:
		Iterator()=default;
		Iterator(const Iterator&)=default;
		bool operator == (const Iterator& b) const { return owner==b.owner && child_idx==b.child_idx; }
		bool operator != (const Iterator& b) const { return !(*this==b); }
		Expr& operator * () const { return owner->_children[child_idx]; }
		Expr& operator -> () const { return owner->_children[child_idx]; }
		Iterator& operator ++ () { ++child_idx; return *this; }
		Iterator operator ++ (int) { return Iterator(owner,child_idx++); }
		Iterator& operator -- () { --child_idx; return *this; }
		Iterator operator -- (int) { return Iterator(owner,child_idx--); }
		friend class Expr;
	};

	class ConstIterator{
		const Expr* owner = nullptr;
		long child_idx = 0;
		ConstIterator(const Expr* owner, long child_idx): owner(owner), child_idx(child_idx) {}
	public:
		ConstIterator()=default;
		ConstIterator(const ConstIterator&)=default;
		bool operator == (const ConstIterator& b) const { return owner==b.owner && child_idx==b.child_idx; }
		bool operator != (const ConstIterator& b) const { return !(*this==b); }
		const Expr& operator * () const { return owner->_children[child_idx]; }
		const Expr& operator -> () const { return owner->_children[child_idx]; }
		ConstIterator& operator ++ () { ++child_idx; return *this; }
		ConstIterator operator ++ (int) { return ConstIterator(owner,child_idx++); }
		ConstIterator& operator -- () { --child_idx; return *this; }
		ConstIterator operator -- (int) { return ConstIterator(owner,child_idx--); }
		friend class Expr;
	};

	Iterator begin() { return Iterator(this,0); }
	Iterator end() { return Iterator(this,_children.size()); }
	ConstIterator begin() const { return ConstIterator(this,0); }
	ConstIterator end() const { return ConstIterator(this,_children.size()); }

	friend string to_string(const Expr& expr);
	friend class _ExprToFromStringImpl;
};



string to_string(const Expr& expr);





struct ExprError : public std::runtime_error{
	Expr subject;
	ExprError(const Expr& subject, string what):std::runtime_error(what),subject(subject){}
};

struct SyntaxError : public std::runtime_error{
	string problem;
	string original;
	size_t position;
	SyntaxError(const string& problem, const string& original, size_t position):
		std::runtime_error("'"+problem+"'\n"+original+"\n"+string(position,' ')+"^here"),
		problem(problem), original(original), position(position) {}
};
