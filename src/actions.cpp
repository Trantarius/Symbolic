#include "actions.hpp"

void recurse_action_bottom_up(Expr& ex, void(*act)(Expr&)){
	for(Expr& child : ex){
		recurse_action_bottom_up(child,act);
	}
	act(ex);
}

void recurse_action_top_down(Expr& ex, void(*act)(Expr&)){
	act(ex);
	for(Expr& child : ex){
		recurse_action_top_down(child,act);
	}
}

void _perform(Expr& expr){
	recurse_action_bottom_up(expr,[](Expr& ex){
		if(ex.type().f_perform!=nullptr)
			ex = ex.type().f_perform(ex,false);
	});
}

void _perform_approx(Expr& expr){
	recurse_action_bottom_up(expr,[](Expr& ex){
		if(ex.type().f_perform!=nullptr)
			ex = ex.type().f_perform(ex,true);
	});
}

const Action& make_perform(){
	static ModAction act(_perform);
	return act;
}
const Action& perform = make_perform();

const Action& make_perform_approx(){
	static ModAction act(_perform_approx);
	return act;
}
const Action& perform_approx = make_perform_approx();
