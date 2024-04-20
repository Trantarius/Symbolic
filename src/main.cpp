#include "main.hpp"

int main (int argc, char **argv){

	std::vector<string> args;
	for(int n=1;n<argc;n++){
		args.push_back(argv[n]);
	}

	Main m(args);
	m.main_loop();

	return m.status;
}

Main::Main(const std::vector<string>& args){
	bool is_input_set=false;
	bool is_output_set=false;
	std::deque<string> positional;
	for(int n=0; n<args.size(); n++){
		if(args[n]=="-in"||args[n]=="-i"){
			if(is_input_set){
				std::cout<<"Cannot specify '-in' more than once"<<std::endl;
				status=1;
				return;
			}
			if(n+1>=args.size()){
				std::cout<<"Expected path after '-in'"<<std::endl;
				status=1;
				return;
			}
			string path = args[n+1];
			file_in.reset(new std::ifstream(path));
			input=file_in.get();
			if(!input->good()){
				std::cout<<"Couldn't open "+path<<std::endl;
				status=1;
				return;
			}
			n++;
			is_input_set=true;
			interactive=false;
		}
		else if(args[n]=="-out"||args[n]=="-o"){
			if(is_output_set){
				std::cout<<"Cannot specify '-out' more than once"<<std::endl;
				status=1;
				return;
			}
			if(n+1>=args.size()){
				std::cout<<"Expected path after '-out'"<<std::endl;
				status=1;
				return;
			}
			string path = args[n+1];
			file_out.reset(new std::ofstream(path));
			output=file_out.get();
			if(!output->good()){
				std::cout<<"Couldn't open "+path<<std::endl;
				status=1;
				return;
			}
			n++;
			is_output_set=true;
		}
		else if(args[n][0]=='-'){
			std::cout<<"Unknown option: '"+args[n]+"'"<<std::endl;
			status=1;
			return;
		}
		else{
			positional.push_back(args[n]);
		}
	}

	if(!is_input_set && !positional.empty()){
		file_in.reset(new std::ifstream(positional.front()));
		input=file_in.get();
		if(!input->good()){
			std::cout<<"Couldn't open "+positional.front()<<std::endl;
			status=1;
			return;
		}
		positional.pop_front();
		interactive=false;
	}

	if(!is_output_set && !positional.empty()){
		file_out.reset(new std::ofstream(positional.front()));
		output=file_out.get();
		if(!output->good()){
			std::cout<<"Couldn't open "+positional.front()<<std::endl;
			status=1;
			return;
		}
		positional.pop_front();
	}
}

struct LineConsumeError : public std::runtime_error{
	using std::runtime_error::runtime_error;
};

void Main::main_loop(){

	int line_num=0;
	while(input->good()){
		if(status!=0){
			break;
		}
		if(interactive)
			print(" > ");

		string line;
		std::getline(*input,line);
		if(std::regex_match(line,std::regex("\\s*"))){
			line_num++;
			continue;
		}

		try{
			consume_line(line);
			if(interactive)
				endl();
		}
		catch(SyntaxError err){
			if(!interactive){
				error(std::to_string(line_num)+"\t"+line);
			}
			error(err.what());
			endl();
		}
		catch(ExprError err){
			if(!interactive){
				error(std::to_string(line_num)+"\t"+line);
			}
			error(err.what());
			error(to_string(err.subject));
			endl();
		}
		catch(std::runtime_error err){
			if(!interactive){
				error(std::to_string(line_num)+"\t"+line);
			}
			error(err.what());
			endl();
		}

		line_num++;
	}
}

void Main::print(string str){
	*output<<str;
}
void Main::error(string str){
	*output<<str<<std::endl;
}
void Main::endl(){
	*output<<std::endl;
}

void Main::consume_line(string line){

	static const std::regex command_rex("\\s*\\$\\s*(\\w+)(?:\\s+(.*))?");
	static const std::regex declare_rex("\\s*(\\w+)\\s*:\\s*(.*)");
	static const std::regex empty_rex("\\s*");

	std::smatch rex_results;

	if(std::regex_match(line,rex_results,command_rex)){
		if(Command::all.contains(rex_results[1])){
			Command& comm = Command::all[rex_results[1]];
			comm.fptr(*this,rex_results[2].str());
		}
		else{
			throw std::runtime_error("Unknown command: "+rex_results[1].str());
		}
	}
	else if(std::regex_match(line,rex_results,declare_rex)){
		if(std::regex_match(rex_results[2].str(),empty_rex)){
			throw std::runtime_error("Expected an expression after ':'");
		}
		else{
			string name = rex_results[1];
			Expr expr (rex_results[2].str());
			if(workspace.contains(name))
				workspace.erase(name);
			workspace.emplace(name,expr);
			if(echo_vars)
				print(name+":\t"+to_string(expr));endl();
		}
	}
	else{
		throw std::runtime_error("Not a declaration or a command");
	}
}
