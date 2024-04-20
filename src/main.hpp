#pragma once

#include <iostream>
#include <fstream>
#include <string>
using std::string;

#include "Expr.hpp"

struct Main;

struct Command{
	inline static std::unordered_map<string,Command> all;
	string name;
	string args;
	string desc;
	void (*fptr)(Main&,string);
	Command()=default;
	Command(string name, string args, string desc):name(name),args(args),desc(desc){
		all.emplace(name,*this);
	}
};

struct Main{
	std::unique_ptr<std::ifstream> file_in;
	std::unique_ptr<std::ofstream> file_out;
	std::istream* input = &std::cin;
	std::ostream* output = &std::cout;
	std::vector<string> history;
	std::unordered_map<string,Expr> workspace;
	bool interactive = true;
	bool echo_vars = true;
	int status=0;

	Main(){}
	Main(const std::vector<string>& args);

	void main_loop();
	void consume_line(string line);

	void print(string);
	void error(string);
	void endl();

};
