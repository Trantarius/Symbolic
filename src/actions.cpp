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

Expr perform(Expr ex){
	recurse_action_bottom_up(ex,[](Expr& ex){
		if(ex.type().f_perform!=nullptr)
			ex = ex.type().f_perform(ex,false);
	});
	return ex;
}

Expr perform_approx(Expr ex){
	recurse_action_bottom_up(ex,[](Expr& ex){
		if(ex.type().f_perform!=nullptr)
			ex = ex.type().f_perform(ex,true);
	});
	return ex;
}
