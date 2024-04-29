#pragma once
#include <string>
#include <cstdint>
#include <set>
using std::string;

#include "FuzzyBool.hpp"

// type used to store all other value types
typedef uint64_t uni_value_t;
typedef int64_t int_value_t;
typedef double float_value_t;
typedef FuzzyBool bool_value_t;
typedef uint64_t hash_t;

class Expr;

class Type{
public:
	const char* name = "";

	// regex to match for parsing; ((?&EXPR)) is a special marker for any sub expression;
	// should always accept possible whitespace to either side
	const char* parse_string = "";

	// regex replace format for printing
	const char* print_string = "";

	// required number of children (exact); INFINITARY for any amount
	enum Arity:uint8_t{NULLARY=0,UNARY=1,BINARY=2,TERNARY=3,INFINITARY=UINT8_MAX};
	Arity arity = NULLARY;

	// determines parsing order and if parentheses are needed for to_string
	// <0 disables parentheses
	int pemdas = 0;

	typedef uint64_t flags_t;
	flags_t flags = 0;

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

	consteval Type()=default;
	Type(const Type&)=delete;
	consteval Type(Type&&)=default;
	Type& operator=(const Type&)=delete;
	Type& operator=(Type&&)=delete;
	bool operator==(const Type& b) const { return this==&b; }
	bool operator!=(const Type& b) const { return this!=&b; }

	Expr operator()() const;
	template<typename...Ts> requires (std::same_as<Expr,Ts>&&...)
	Expr operator()(Ts...args) const;
};

inline string to_string(const Type& type){ return type.name; }

template<typename T> struct ValueType : public Type{
	consteval ValueType()=default;
	Expr operator()(T v) const;
	T value(const Expr& ex) const;
};
template struct ValueType<string>;
template struct ValueType<int_value_t>;
template struct ValueType<float_value_t>;
template struct ValueType<bool_value_t>;

// any comma seperated list. means nothing until a type with flag LIST_UNWRAPPER unwraps it.
extern const Type List;

// TODO
extern const Type IfElse;
extern const Type ForIn;

// TODO
extern const Type And;
extern const Type Or;
extern const Type Not;

// TODO
extern const Type IsIn;

// TODO
extern const Type Equal;
extern const Type NotEqual;
extern const Type Less;
extern const Type Greater;
extern const Type LessEqual;
extern const Type GreaterEqual;

extern const Type Add;
extern const Type Sub;
extern const Type Mul;
extern const Type Div;
extern const Type Neg;
extern const Type Pow;

// TODO
extern const Type Vector;
extern const Type Set;
extern const Type Tuple;

// TODO
extern const Type Sin;
extern const Type Cos;
extern const Type Tan;
extern const Type ASin;
extern const Type ACos;
extern const Type ATan;
extern const Type SinH;
extern const Type CosH;
extern const Type TanH;

// TODO
extern const Type Pi;
extern const Type Euler;
extern const Type Imaginary;

extern const ValueType<bool_value_t> Bool;
extern const ValueType<string> Symbol;
extern const ValueType<float_value_t> Float;
extern const ValueType<int_value_t> Integer;

extern const Type Undefined;

struct cmp_types{
	bool operator()(const Type* a, const Type* b) const {
		return a->pemdas > b->pemdas;
	}
};
inline std::multiset<const Type*,cmp_types> all_types;

#define REGISTER_TYPE(TYPE) \
const int _register_##TYPE = [](){all_types.emplace(&TYPE);return 0;}()
