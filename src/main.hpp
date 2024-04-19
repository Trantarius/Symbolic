#pragma once

#include <iostream>
#include <string>
using std::string;

#include "Expr.hpp"

struct Main{
	std::istream& input;
	std::ostream& output;
	std::unordered_map<string,Expr> environment;
	bool interactive = false;

	Main():input(std::cin),output(std::cout),interactive(true){}
	Main(std::istream& input,std::ostream& output):input(input),output(output){}

	void main_loop();
	void consume_line(string line);

};
