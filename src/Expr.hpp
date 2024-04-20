#pragma once

#include <string>
#include <deque>
#include <unordered_map>
#include <set>
#include <regex>

using std::string;

struct Expr {

	class Type{
		inline static uint64_t last_id=0;
		struct cmp_types{
			bool operator()(const Type* a, const Type* b) const {
				return a->pemdas > b->pemdas;
			}
		};
	public:
		inline static std::multiset<const Type*,cmp_types> all_known_types;
		uint64_t id=last_id++ * 313 + 727;
		string name;
		std::regex parser;
		string printer;
		std::vector<string> operators;
		enum Arity{NULLARY,UNARY,BINARY,TERNARY,INFINITARY};
		Arity arity;
		// determines parsing order and if parentheses are needed for to_string
		// <0 disables parentheses
		int pemdas;
		Type()=delete;
		Type(const Type&)=delete;
		Type(string name, string parser, string printer, std::initializer_list<string> operators,
				 Arity arity, int pemdas): name(name), parser(parser), printer(printer),
				 operators(operators), arity(arity),pemdas(pemdas){
				all_known_types.insert(this);
			};
		bool operator==(const Type& b) const { return id==b.id; }
		bool operator!=(const Type& b) const { return id!=b.id; }
		Expr operator()() const;
		Expr operator()(const std::initializer_list<Expr>& il) const;
	};

	inline static const Type List{"List", "(.*?),(.*)", "$1,$2", {","}, Type::INFINITARY,1000};
	inline static const Type Add{"Add", "(.*?)\\+(.*)", "$1+$2", {"+"}, Type::INFINITARY,60};
	inline static const Type Sub{"Sub", "(.*?)-(.*)", "$1-$2", {"-"}, Type::BINARY,50};
	inline static const Type Mul{"Mul", "(.*?)\\*(.*)", "$1*$2", {"*"}, Type::INFINITARY,40};
	inline static const Type Div{"Div", "(.*?)/(.*)", "$1/$2", {"/"}, Type::BINARY,30};
	inline static const Type Neg{"Neg", "\\s*-(.*)", "-$1", {"-"}, Type::UNARY,20};
	inline static const Type Pow{"Pow", "(.*)\\^(.*)", "$1^$2", {"^"}, Type::BINARY,10};
	inline static const Type Symbol{"Symbol", "\\s*(\\w+)\\s*", "", {}, Type::NULLARY,-10};
	inline static const Type Number{"Number", "\\s*(\\d+)\\s*", "", {}, Type::NULLARY,-10};
	inline static const Type Undefined{"Undefined", "", "∅", {},Type::NULLARY,-1000};

private:
	inline static std::vector<string> symbol_names;
	inline static std::unordered_map<string,long> symbol_ids;
	const Type* _type = &Undefined;
	std::deque<Expr> _children;
	int64_t _value=0;
public:

	Expr()=default;
	Expr(const Expr&)=default;
	Expr(Expr&& ex):_children(std::move(ex._children)),_type(ex._type),_value(ex._value){
		ex._type=&Undefined;
	}
	Expr(const Type& type);
	Expr(const Type& type, const std::initializer_list<Expr>& il);
	Expr(string str);

	const Type& type() const {return *_type; }
	uint64_t hash() const;
	bool is_identical(const Expr& epxr) const;

	Expr& operator=(const Expr&)=default;
	Expr& operator=(Expr&& b) {
		_type=b._type;
		_children=std::move(b._children);
		_value = b._value;
		return *this;
	};

	Expr& operator[](long n);
	const Expr& operator[](long n) const;
	long child_count() const { return _children.size(); }
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

	friend bool string_to_expr_type(string str, const std::vector<string>& bracketed, const Expr::Type& type, Expr& out);
	friend string to_string(const Expr& expr);
};



string to_string(const Expr& expr);





struct ExprError : public std::runtime_error{
	Expr subject;
	ExprError(const Expr& subject, string what):std::runtime_error(what),subject(subject){}
};

struct SyntaxError : public std::runtime_error{
	using std::runtime_error::runtime_error;
};
struct MissingOperand : public SyntaxError{
protected:
	MissingOperand(string which, string oper):SyntaxError("missing "+which+" operand for "+oper){}
public:
	MissingOperand(string oper):SyntaxError("missing operand for "+oper){}
};
struct MissingLeftOperand : public MissingOperand{
	MissingLeftOperand(string oper):MissingOperand("left",oper){} };
struct MissingCenterOperand : public MissingOperand{
	MissingCenterOperand(string oper):MissingOperand("center",oper){} };
struct MissingRightOperand : public MissingOperand{
	MissingRightOperand(string oper):MissingOperand("right",oper){} };
