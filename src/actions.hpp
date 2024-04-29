#pragma once
#include "Expr.hpp"

struct Action{
	virtual Expr operator()(const Expr&) const = 0;
	virtual Expr operator()(Expr&&) const = 0;
	virtual void in_place(Expr&) const = 0;
};

struct RefAction : public Action{
	Expr (* const fptr)(const Expr&);
	virtual Expr operator()(const Expr& ex) const override { return fptr(ex); }
	virtual Expr operator()(Expr&& ex) const override { return fptr(ex); }
	virtual void in_place(Expr& ex) const override { ex = fptr(ex); }
	RefAction(Expr(*fptr)(const Expr&)):fptr(fptr){}
};

struct CopyAction : public Action{
	Expr (* const fptr)(Expr);
	virtual Expr operator()(const Expr& ex) const override { return fptr(ex); }
	virtual Expr operator()(Expr&& ex) const override { return fptr(std::move(ex)); }
	virtual void in_place(Expr& ex) const override { ex = fptr(std::move(ex)); }
	CopyAction(Expr(*fptr)(Expr)):fptr(fptr){}
};

struct ModAction : public Action{
	void (* const fptr)(Expr&);
	virtual Expr operator()(const Expr& ex) const override {
		Expr expr = ex;
		fptr(expr);
		return expr;
	}
	virtual Expr operator()(Expr&& ex) const override {
		fptr(ex);
		return ex;
	}
	virtual void in_place(Expr& ex) const override { fptr(ex); }
	ModAction(void(*fptr)(Expr&)):fptr(fptr){}
};

struct Rule : public Action{
	Expr left,right;
	virtual Expr operator()(const Expr& ex) const override;
	virtual Expr operator()(Expr&& ex) const override;
	virtual void in_place(Expr& ex) const override;
};

extern const Action& perform;
extern const Action& perform_approx;
