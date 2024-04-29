#include "main.hpp"

#include <boost/regex.hpp>

const boost::regex empty_rex("\\s*");
const boost::regex arg_rex("\\S+");
const boost::regex positive_rex("(?:yes)|(?:true)|(?:on)|(?:enable)");
const boost::regex negative_rex("(?:no)|(?:false)|(?:off)|(?:disable)");

std::vector<string> split_args(const string& str){
	std::vector<string> ret;
	boost::sregex_iterator iter(str.begin(),str.end(),arg_rex);
	while(iter!=boost::sregex_iterator())
		ret.push_back((iter++)->str());
	return ret;
}

void help(Main& main, string args){
	boost::smatch rex_result;
	if(boost::regex_match(args,empty_rex)){
		for(const std::pair<string,Command>& comm : Command::all){
			main.output<<comm.second.name<<" "<<comm.second.args<<endl;
		}
	}
	else{
		std::vector<string> argv = split_args(args);
		for(const string& arg : argv){
			if(Command::all.contains(arg)){
				const Command& comm = Command::all[arg];
				main.output<<comm.name<<" "<<comm.args<<endl;
				main.output<<"\t"<<comm.desc<<endl;
			}
			else{
				main.output<<"There is no command named '"<<arg<<"'"<<endl;
			}
		}
	}
}

Command help_command("help","[command...]","Prints a list of all commands. If given the name of a command(s), it will print their description(s).",help);

void show(Main& main, string args){
	std::vector<string> argv = split_args(args);
	if(argv.size()==0){
		for(const std::pair<string,Expr>& named_expr : main.workspace){
			main.output<<named_expr.first<<":\t"<<to_string(named_expr.second)<<endl;
		}
	}
	else{
		for(const string& name : argv){
			if(main.workspace.contains(name)){
				const Expr& expr = main.workspace[name];
				main.output<<name<<":\t"<<to_string(expr)<<endl;
			}
			else{
				main.output<<"There is no expression named '"<<name<<"'"<<endl;
			}
		}
	}
}

Command show_command("show","[name...]","Prints a named expression(s). If no names are given, all named expressions are printed.",show);

void delete_c(Main& main, string args){
	std::vector<string> argv = split_args(args);
	if(argv.size()==0){
		main.workspace.clear();
	}
	else{
		for(const string& name : argv){
			if(main.workspace.contains(name)){
				main.workspace.erase(name);
			}
			else{
				main.output<<"There is no expression named '"<<name<<"'"<<endl;
			}
		}
	}
}

Command delete_command("delete","[name...]","Deletes a named expression(s). If no names are given, all named expressions are deleted.",delete_c);

void echo(Main& main, string args){
	std::vector<string> argv = split_args(args);
	if(args.size()>1)
		throw CommandError("Expected 'on' or 'off'");
	if(boost::regex_match(args,empty_rex)){
		main.echo_vars = ! main.echo_vars;
	}
	string arg = argv[0];
	if(boost::regex_match(arg,positive_rex)){
		main.echo_vars=true;
	}
	else if(boost::regex_match(arg,negative_rex)){
		main.echo_vars=false;
	}
	else{
		throw CommandError("Expected 'on' or 'off'");
	}
}

Command echo_command("echo","[on|off]", "Toggles the printing of newly created named expressions.",echo);

void _showtree(const Expr& expr, std::ostream& out, const string& prefix, const string& ext, string prefix_override=""){
	if(prefix_override=="")
		out<<prefix<<ext;
	else
		out<<prefix_override<<ext;
	if(expr.type().arity==Type::NULLARY)
		out<<to_string(expr)<<endl;
	else
		out<<expr.type().name<<endl;
	string next_prefix=prefix;
	if(ext==" ├╴")
		next_prefix+=" │ ";
	else
		next_prefix+="   ";

	for(size_t n=0;n<expr.child_count();n++){
		if(n==expr.child_count()-1)
			_showtree(expr[n],out,next_prefix," ╰╴");
		else
			_showtree(expr[n],out,next_prefix," ├╴");
	}
}

void showtree(Main& main, string args){
	constexpr size_t indent = 4;
	std::vector<string> argv = split_args(args);
	if(argv.size()==0){
		for(const std::pair<string,Expr>& named_expr : main.workspace){
			main.output<<named_expr.first<<":";
			_showtree(named_expr.second,main.output,string(indent+named_expr.first.size()+1-3,' '),"",string(indent,' '));
		}
	}
	else{
		for(const string& name : argv){
			if(main.workspace.contains(name)){
				main.output<<name<<":";
				_showtree(main.workspace[name],main.output,string(indent+name.size()+1-3,' '),"",string(indent,' '));
			}
			else{
				main.output<<"There is no expression named '"<<name<<"'"<<endl;
			}
		}
	}
}

Command showtree_command("showtree","[name...]","Prints a named expression(s) as a tree. If no names are given, all named expressions are printed.",showtree);
