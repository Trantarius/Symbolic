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
		enum Arity:uint8_t{NULLARY=0,UNARY=1,BINARY=2,TERNARY=3,INFINITARY=UINT8_MAX};
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
		friend uint64_t Expr::hash() const;
	};

	inline static const Type _Undefined{"Undefined", "", "∅", Type::NULLARY, -1000};

private:
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
	bool is_identical(const Expr& expr) const;

	Expr& operator=(const Expr&)=default;
	Expr& operator=(Expr&& b) {
		_type=b._type;
		_children=std::move(b._children);
		_value = b._value;
		return *this;
	};
	bool operator==(const Expr& expr) const{ return is_identical(expr); }

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
};

struct InfinitaryExprType : public Expr::Type{
	InfinitaryExprType(const string& name, const string& parser, const string& printer, int pemdas):
		Expr::Type(name,parser,printer,INFINITARY,pemdas){}

	template<typename...Ts> requires (std::same_as<Expr,Ts>&&...)
	Expr operator()(Ts...args) const{
		return Expr{*this,{std::move(args)...}};
	}
};

struct TernaryExprType : public Expr::Type{
	TernaryExprType(const string& name, const string& parser, const string& printer, int pemdas):
		Expr::Type(name,parser,printer,TERNARY,pemdas){}

	Expr operator()() const {
		return Expr(*this);
	}
	Expr operator()(Expr a,Expr b,Expr c) const{
		return Expr(*this,{std::move(a),std::move(b),std::move(c)});
	}
};

struct BinaryExprType : public Expr::Type{
	BinaryExprType(const string& name, const string& parser, const string& printer, int pemdas):
		Expr::Type(name,parser,printer,BINARY,pemdas){}

	Expr operator()() const {
		return Expr(*this);
	}
	Expr operator()(Expr a,Expr b) const{
		return Expr(*this,{std::move(a),std::move(b)});
	}
};

struct UnaryExprType : public Expr::Type{
	UnaryExprType(const string& name, const string& parser, const string& printer, int pemdas):
		Expr::Type(name,parser,printer,UNARY,pemdas){}

	Expr operator()() const {
		return Expr(*this);
	}
	Expr operator()(Expr a) const{
		return Expr{*this,{std::move(a)}};
	}
};

struct NullaryExprType : public Expr::Type{
	NullaryExprType(const string& name, const string& parser, const string& printer, int pemdas):
		Expr::Type(name,parser,printer,NULLARY,pemdas){}

	Expr operator()() const {
		return Expr(*this);
	}
};

struct SymbolExprType : public Expr::Type{
	SymbolExprType(const string& name, const string& parser, const string& printer, int pemdas):
		Expr::Type(name,parser,printer,NULLARY,pemdas){}

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
	NumberExprType(const string& name, const string& parser, const string& printer, int pemdas):
		Expr::Type(name,parser,printer,NULLARY,pemdas){}

	Expr operator()(long num) const {
		Expr ret{*this};
		ret._value = num;
		return ret;
	}
};

inline const InfinitaryExprType List{"List", R"(((?&EXPR)),((?&EXPR)))", "$1, $2", 1000};
inline const InfinitaryExprType Add{"Add", R"(((?&EXPR))\+((?&EXPR)))", "$1 + $2", 60};
inline const BinaryExprType Sub{"Sub",
	R"(((?:(?&EXPR)\-)*(?&EXPR))(?<=[\w\)\]\}])\s*\-((?&EXPR)))", "$1 - $2", 50};
inline const InfinitaryExprType Mul{"Mul", R"(((?&EXPR))\*((?&EXPR)))", "$1*$2", 40};
inline const BinaryExprType Div{"Div", R"(((?:(?&EXPR)/)*(?&EXPR))/((?&EXPR)))", "$1/$2", 30};
inline const UnaryExprType Neg{"Neg", R"(\s*-((?&EXPR)))", "-$1", 20};
inline const BinaryExprType Pow{"Pow", R"(((?&EXPR))\^((?&EXPR)(?:\^(?&EXPR))*))", "$1^$2", 10};
inline const SymbolExprType Symbol{"Symbol", R"(\s*([a-zA-Z_]+)\s*)", "", -10};
inline const NumberExprType Number{"Number", R"(\s*([0-9]+)\s*)", "", -10};
inline const Expr::Type& Undefined=Expr::_Undefined;

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
