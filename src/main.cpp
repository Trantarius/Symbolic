#include "main.hpp"

int main (int argc, char **argv){
	int status=0;

	Main m;
	m.main_loop();

	return status;
}

void Main::main_loop(){
	while(!input.eof()){
		string line;

		if(interactive)
			output<<" > ";
		std::getline(input,line);
		try{
			consume_line(std::move(line));
		}
		catch(SyntaxError err){
			output<<err.what()<<std::endl;
		}
	}
}

void Main::consume_line(string line){
	Expr expr(line);
	output<<to_string(expr)<<std::endl;
}
