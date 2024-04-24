#pragma once

#include <string>
using std::string;

#include "Expr.hpp"

#include <xeus/xinterpreter.hpp>
#include <nlohmann/json.hpp>

using std::endl;

struct Main;

struct Command{
	inline static std::unordered_map<string,Command> all;
	string name;
	string args;
	string desc;
	void (*fptr)(Main&,string);
	Command()=default;
	Command(string name, string args, string desc,void(*fptr)(Main&,string)):name(name),args(args),desc(desc),fptr(fptr){
		all.emplace(name,*this);
	}
};

struct CommandError : public NamedError{
	CommandError(string what):NamedError("CommandError",std::move(what)){}
};

struct Main : public xeus::xinterpreter{

	std::ostringstream output;
	std::map<string,Expr> workspace;
	bool echo_vars = true;
	void consume_line(string line);


	// functions for jupyter:

	void configure_impl() override;
	void execute_request_impl(xeus::xrequest_context request_context,
														send_reply_callback cb,
														int execution_counter,
														const std::string& code,
														xeus::execute_request_config config,
														nl::json user_expressions) override;
	nl::json complete_request_impl(const std::string& code, int cursor_pos) override;
	nl::json inspect_request_impl(const std::string& code,
																int cursor_pos,
																int detail_level) override;
	nl::json is_complete_request_impl(const std::string& code) override;
	nl::json kernel_info_request_impl() override;
	void shutdown_request_impl() override;
};
