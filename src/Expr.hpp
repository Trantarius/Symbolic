#pragma once

#include <string>
#include <deque>
#include <unordered_map>
#include <set>
#include <cstdint>
#include <vector>
#include <stdexcept>

using std::string;



struct Expr {

	uint64_t hash() const;

	class Type{
		struct cmp_types{
			bool operator()(const Type* a, const Type* b) const {
				return a->pemdas > b->pemdas;
			}
		};
		inline static uint8_t _fc = 0;
	public:
		inline static std::multiset<const Type*,cmp_types> all_types;
		string name;

		// regex to match for parsing; ((?&EXPR)) is a special marker for any sub expression
		string parse_string;

		// regex replace format for printing
		string print_string;

		// required number of children (exact); INFINITARY for any amount
		enum Arity:uint8_t{NULLARY=0,UNARY=1,BINARY=2,TERNARY=3,INFINITARY=UINT8_MAX};
		Arity arity;

		// determines parsing order and if parentheses are needed for to_string
		// <0 disables parentheses
		int pemdas;

		typedef uint16_t flags_t;
		flags_t flags;

		// if it can be performed (e.g. Add, Mul, cosine, etc. but not List, Symbol, etc.)
		inline static const flags_t OPERATOR = 1<<(_fc++);
		bool is_operator() const { return flags&OPERATOR; }

		// if it contains a comma-seperated list. a List child will be unnested into this during parsing.
		inline static const flags_t LIST_UNWRAPPER = 1<<(_fc++);
		bool is_list_unwrapper() const { return flags&LIST_UNWRAPPER; }

		// if this type uses the _value property of Expr
		inline static const flags_t USES_VALUE = 1<<(_fc++);
		bool uses_value() const { return flags&USES_VALUE; }

		// if this represents a collection of values, like a vector or set
		inline static const flags_t COLLECTION = 1<<(_fc++);
		bool is_collection() const { return flags&COLLECTION; }

		// is a known value, such as a number, pi, i, etc.
		inline static const flags_t CONSTANT = 1<<(_fc++);
		bool is_constant() const { return flags&CONSTANT; }

		// if order of children is meaningless
		inline static const flags_t COMMUTATIVE = 1<<(_fc++);
		bool is_commutative() const { return flags&COMMUTATIVE; }

		//if children of the same type can be unnested into this (this should be infinitary)
		inline static const flags_t ASSOCIATIVE = 1<<(_fc++);
		bool is_associative() const { return flags&ASSOCIATIVE; }

		// if it can be applied to a collection on a per-element basis (exact behavior depends on the collection)
		inline static const flags_t ENTRYWISE = 1<<(_fc++);
		bool is_entrywise() const { return flags&ASSOCIATIVE; }

		// represents a relationship b/w children. if it's an operator, it will return a bool
		inline static const flags_t RELATIONSHIP = 1<<(_fc++);
		bool is_relationship() const { return flags&RELATIONSHIP; }

		// if boolean values are used for input and output
		inline static const flags_t BOOLEAN = 1<<(_fc++);
		bool is_boolean() const { return flags&BOOLEAN; }

		Type()=default;
		Type(const Type&)=delete;
		bool operator==(const Type& b) const { return this==&b; }
		bool operator!=(const Type& b) const { return this!=&b; }
		friend uint64_t Expr::hash() const;
	};

private:
	static const Type& _Undefined;//{"Undefined", "", "∅", Type::NULLARY, -1000, 0};

	inline static std::vector<string> symbol_names;
	inline static std::unordered_map<string,long> symbol_ids;
	const Type* _type = &_Undefined;
	std::deque<Expr> _children;
	int64_t _value=0;

	Expr(const Type& type);
	Expr(const Type& type, const std::initializer_list<Expr>& il);
public:

	Expr()=default;
	Expr(const Expr&)=default;
	Expr(Expr&& ex):_type(ex._type),_children(std::move(ex._children)),_value(ex._value){
		ex._type=&_Undefined;
	}
	Expr(const string& str);

	const Type& type() const {return *_type; }
	int64_t value() const { return _value; }
	bool is_identical_to(const Expr& expr) const;

	Expr& operator=(const Expr&)=default;
	Expr& operator=(Expr&& b) {
		_type=b._type;
		_children=std::move(b._children);
		_value = b._value;
		return *this;
	};
	bool operator==(const Expr& expr) const{ return is_identical_to(expr); }

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

	friend class _ExprToFromStringImpl;
	friend class InfinitaryExprType;
	friend class TernaryExprType;
	friend class BinaryExprType;
	friend class UnaryExprType;
	friend class NullaryExprType;
	friend class SymbolExprType;
	friend class NumberExprType;

	friend string to_string(const Expr& expr);
	friend string symbol_printer(const Expr& ex);
	friend string number_printer(const Expr& ex);
};

template<>
struct std::hash<Expr>{
	uint64_t operator ()(const Expr& ex) const {
		return ex.hash();
	}
};

struct InfinitaryExprType : public Expr::Type{

	template<typename...Ts> requires (std::same_as<Expr,Ts>&&...)
	Expr operator()(Ts...args) const{
		return Expr{*this,{std::move(args)...}};
	}
};

struct TernaryExprType : public Expr::Type{

	Expr operator()() const {
		return Expr(*this);
	}
	Expr operator()(Expr a,Expr b,Expr c) const{
		return Expr(*this,{std::move(a),std::move(b),std::move(c)});
	}
};

struct BinaryExprType : public Expr::Type{

	Expr operator()() const {
		return Expr(*this);
	}
	Expr operator()(Expr a,Expr b) const{
		return Expr(*this,{std::move(a),std::move(b)});
	}
};

struct UnaryExprType : public Expr::Type{

	Expr operator()() const {
		return Expr(*this);
	}
	Expr operator()(Expr a) const{
		return Expr{*this,{std::move(a)}};
	}
};

struct NullaryExprType : public Expr::Type{

	Expr operator()() const {
		return Expr(*this);
	}
};

struct SymbolExprType : public Expr::Type{

	Expr operator()(const string& name) const {
		Expr ret{*this};
		if(Expr::symbol_ids.contains(name)){
			ret._value=Expr::symbol_ids[name];
		}
		else{
			ret._value=Expr::symbol_names.size();
			Expr::symbol_names.push_back(name);
			Expr::symbol_ids.emplace(name,ret._value);
		}
		return ret;
	}
};

struct NumberExprType : public Expr::Type{

	Expr operator()(long num) const {
		Expr ret{*this};
		ret._value = num;
		return ret;
	}
};


// any comma seperated list. means nothing until a type with flag LIST_UNWRAPPER unwraps it.
extern const InfinitaryExprType& List;

// TODO
extern const TernaryExprType& IfElse;
extern const TernaryExprType& ForIn;

// TODO
extern const BinaryExprType& And;
extern const BinaryExprType& Or;
extern const UnaryExprType& Not;

// TODO
extern const BinaryExprType& Equal;
extern const BinaryExprType& NotEqual;
extern const BinaryExprType& Less;
extern const BinaryExprType& Greater;
extern const BinaryExprType& LessEqual;
extern const BinaryExprType& GreaterEqual;

extern const InfinitaryExprType& Add;
extern const BinaryExprType& Sub;
extern const InfinitaryExprType& Mul;
extern const BinaryExprType& Div;
extern const UnaryExprType& Neg;
extern const BinaryExprType& Pow;

// TODO
extern const InfinitaryExprType& Vector;
extern const InfinitaryExprType& Set;
extern const InfinitaryExprType& Tuple;

// TODO
extern const UnaryExprType& Sin;
extern const UnaryExprType& Cos;
extern const UnaryExprType& Tan;
extern const UnaryExprType& ASin;
extern const UnaryExprType& ACos;
extern const UnaryExprType& ATan;
extern const UnaryExprType& SinH;
extern const UnaryExprType& CosH;
extern const UnaryExprType& TanH;

// TODO
extern const NullaryExprType& Pi;
extern const NullaryExprType& Euler;
extern const NullaryExprType& Imaginary;

extern const SymbolExprType& Symbol;
extern const NumberExprType& Number;

extern const Expr::Type& Undefined;

string to_string(const Expr& expr);



struct NamedError : public std::runtime_error{
	string name;
	NamedError(const string& name, const string& what):std::runtime_error(what),name(name){}
};

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
