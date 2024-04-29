#pragma once

#include <string>
#include <deque>
#include <set>
#include <cstdint>

#include "error.hpp"
#include "FuzzyBool.hpp"

using std::string;

// type used to store all other value types
typedef uint64_t uni_value_t;
typedef int64_t int_value_t;
typedef double float_value_t;
typedef FuzzyBool bool_value_t;

typedef uint64_t hash_t;

struct Expr {

	hash_t hash() const;

	class Type{
		struct cmp_types{
			bool operator()(const Type* a, const Type* b) const {
				return a->pemdas > b->pemdas;
			}
		};
	public:
		inline static std::multiset<const Type*,cmp_types> all_types;
		string name;

		// regex to match for parsing; ((?&EXPR)) is a special marker for any sub expression;
		// should always accept possible whitespace to either side
		string parse_string;

		// regex replace format for printing
		string print_string;

		// required number of children (exact); INFINITARY for any amount
		enum Arity:uint8_t{NULLARY=0,UNARY=1,BINARY=2,TERNARY=3,INFINITARY=UINT8_MAX};
		Arity arity;

		// determines parsing order and if parentheses are needed for to_string
		// <0 disables parentheses
		int pemdas;

		typedef uint64_t flags_t;
		flags_t flags;

	private:
		static constexpr int _fcstart = __COUNTER__+1;
#define NEXT (__COUNTER__-_fcstart)
	public:

		// if it contains a comma-seperated list. a List child will be unnested into this during parsing.
		constexpr static const flags_t LIST_UNWRAPPER = 1<<NEXT;
		bool is_list_unwrapper() const { return flags&LIST_UNWRAPPER; }

		// if this type uses the _value property of Expr
		constexpr static const flags_t VALUE_TYPE = 1<<NEXT;
		bool is_value_type() const { return flags&VALUE_TYPE; }

		// is a known value, such as a number, pi, i, etc.
		constexpr static const flags_t CONSTANT = 1<<NEXT;
		bool is_constant() const { return flags&CONSTANT; }

		// if this is a collection like a vector or set
		constexpr static const flags_t COLLECTION = 1<<NEXT;
		bool is_collection() const { return flags&COLLECTION; }

		// if order of children is meaningless
		constexpr static const flags_t COMMUTATIVE = 1<<NEXT;
		bool is_commutative() const { return flags&COMMUTATIVE; }

		// if children of the same type can be unnested into this (this should be infinitary);
		// also allows a sub-sequence of members to be calculated when not all members are known values
		constexpr static const flags_t ASSOCIATIVE = 1<<NEXT;
		bool is_associative() const { return flags&ASSOCIATIVE; }

		// if it can be applied to a collection on a per-element basis
		// (exact behavior depends on the collection)
		constexpr static const flags_t ENTRYWISE = 1<<NEXT;
		bool is_entrywise() const { return flags&ASSOCIATIVE; }
#undef NEXT

		// any of the following function pointers may be null if that operation is not supported

		// custom parsing function; receives a string that matched its parse_string
		typedef Expr (*f_parser_t)(const string&);
		f_parser_t f_parser=nullptr;

		// custom printing function
		typedef string (*f_printer_t)(const Expr&);
		f_printer_t f_printer=nullptr;

		// non-recursively dissolve this expr (ie, do addition, resolve ifs, etc);
		// returns self if not currently possible
		typedef Expr (*f_perform_t)(const Expr&,bool allow_approx);
		f_perform_t f_perform=nullptr;

		// gets an int representation of this value (only for constants)
		typedef int_value_t (*f_get_int_t)(const Expr&);
		f_get_int_t f_get_int=nullptr;

		// gets a float representation of this value (only for constants)
		typedef float_value_t (*f_get_float_t)(const Expr&);
		f_get_float_t f_get_float=nullptr;

		// gets a bool representation of this value (only for constants)
		typedef bool_value_t (*f_get_bool_t)(const Expr&);
		f_get_bool_t f_get_bool=nullptr;

		Type()=default;
		Type(const Type&)=delete;
		Type(Type&&)=delete;
		Type& operator=(const Type&)=delete;
		Type& operator=(Type&&)=delete;
		bool operator==(const Type& b) const { return this==&b; }
		bool operator!=(const Type& b) const { return this!=&b; }

		virtual Expr operator()() const;
		friend uint64_t Expr::hash() const;
	};

private:
	static const Type& _Undefined;
	const Type* _type = &_Undefined;
	std::deque<Expr> _children;
	uni_value_t _value=0;

	//Expr(const Type& type);
	//Expr(const Type& type, const std::initializer_list<Expr>& il);
	// for Type to directly initialize an Expr
	Expr(const Type* _t, std::deque<Expr>&& _c, uni_value_t _v):
		_type(_t), _children(std::move(_c)), _value(_v) {}
public:

	Expr()=default;
	Expr(const Expr&)=default;
	Expr(Expr&& ex):_type(ex._type),_children(std::move(ex._children)),_value(ex._value){
		ex._type=&_Undefined;
	}
	Expr(const string& str);
	Expr(int_value_t);
	Expr(float_value_t);
	Expr(bool_value_t);

	const Type& type() const {return *_type; }
	bool is_identical_to(const Expr& expr) const;

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
	friend class InfinitaryExprType;
	friend class TernaryExprType;
	friend class BinaryExprType;
	friend class UnaryExprType;
	friend class NullaryExprType;
	template<typename T>
	friend class ValueExprType;

};

string to_string(const Expr& expr);
inline string to_string(const Expr::Type& type){ return type.name; }



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

struct InfinitaryExprType : public Expr::Type{
	InfinitaryExprType(){
		arity = Expr::Type::INFINITARY;
	}

	template<typename...Ts> requires (std::same_as<Expr,Ts>&&...)
	Expr operator()(Ts...args) const{
		return Expr(this,{std::move(args)...},0);
	}
};

struct TernaryExprType : public Expr::Type{
	TernaryExprType(){
		arity = Expr::Type::TERNARY;
	}
	Expr operator()(Expr a,Expr b,Expr c) const{
		return Expr(this,{std::move(a),std::move(b),std::move(c)},0);
	}
};

struct BinaryExprType : public Expr::Type{
	BinaryExprType(){
		arity = Expr::Type::BINARY;
	}
	Expr operator()(Expr a,Expr b) const{
		return Expr(this,{std::move(a),std::move(b)},0);
	}
};

struct UnaryExprType : public Expr::Type{
	UnaryExprType(){
		arity = Expr::Type::UNARY;
	}
	Expr operator()(Expr a) const{
		return Expr(this,{std::move(a)},0);
	}
};

struct NullaryExprType : public Expr::Type{
	NullaryExprType(){
		arity = Expr::Type::NULLARY;
	}
};

template<typename T> struct ValueExprType : public Expr::Type{
private:
	static uni_value_t store(T v);
	static T retrieve(uni_value_t v);
public:
	ValueExprType(){
		arity = Expr::Type::NULLARY;
		flags |= Expr::Type::VALUE_TYPE;
	}
	Expr operator()(T v) const{
		return Expr(this, {}, store(v));
	}
	T value(const Expr& ex) const{
		ASSERT_EQUAL(ex.type(),*this);
		return retrieve(ex._value);
	}
};
template struct ValueExprType<string>;
template struct ValueExprType<int_value_t>;
template struct ValueExprType<float_value_t>;
template struct ValueExprType<bool_value_t>;

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
extern const BinaryExprType& IsIn;

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

extern const ValueExprType<bool_value_t>& Bool;
extern const ValueExprType<string>& Symbol;
extern const ValueExprType<float_value_t>& Float;
extern const ValueExprType<int_value_t>& Integer;

extern const Expr::Type& Undefined;

