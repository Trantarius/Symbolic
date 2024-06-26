#include "main.hpp"

#include <xeus/xkernel.hpp>
#include <xeus/xkernel_configuration.hpp>
#include <xeus-zmq/xserver_zmq.hpp>
#include <xeus-zmq/xzmq_context.hpp>
#include <xeus/xhelper.hpp>
#include <boost/regex.hpp>

#ifdef TEST_BUILD
#include "tests.hpp"

int main(){
	bool all_passed=true;
	for(Test* test : Test::all_tests){
		std::cout<<test->name;
		std::cout.flush();
		test->run();
		if(test->status==Test::FAILED){
			std::cout<<" failed: "<<test->fail_reason<<std::endl;
			all_passed=false;
		}else{
			std::cout<<"\r"<<string(test->name.size(),' ')<<"\r";
		}
	}

	if(all_passed){
		std::cout<<"All "<<std::to_string(Test::all_tests.size())<<" tests passed"<<std::endl;
	}
}

#else

int main (int argc, char **argv){

	if(argc<2){
		std::cerr<<"Expected connection file path"<<std::endl;
		return 1;
	}

	std::unique_ptr<Main> mptr(new Main());
	xeus::xconfiguration config = xeus::load_configuration(argv[1]);

	auto context = xeus::make_zmq_context();

	// Create kernel instance and start it
	xeus::xkernel kernel(config, xeus::get_user_name(), std::move(context), std::move(mptr), xeus::make_xserver_default);
	kernel.start();
	return 0;
}

#endif

void Main::consume_line(string line){

	static const boost::regex command_rex("\\s*\\$\\s*(\\w+)(?:\\s+(.*))?");
	static const boost::regex declare_rex("\\s*(\\w+)\\s*:\\s*(.*)");
	static const boost::regex empty_rex("\\s*");

	boost::smatch rex_results;

	if(boost::regex_match(line,rex_results,command_rex)){
		if(Command::all.contains(rex_results[1])){
			Command& comm = Command::all[rex_results[1]];
			comm.fptr(*this,rex_results[2].str());
		}
		else{
			throw CommandError("Unknown command: "+rex_results[1].str());
		}
	}
	else if(boost::regex_match(line,rex_results,declare_rex)){
		if(boost::regex_match(rex_results[2].str(),empty_rex)){
			throw CommandError("Expected an expression after ':'");
		}
		else{
			string name = rex_results[1];
			Expr expr (rex_results[2].str());
			if(workspace.contains(name))
				workspace.erase(name);
			workspace.emplace(name,expr);
			if(echo_vars)
				output<<(name+":\t"+to_string(expr))<<endl;
		}
	}
	else{
		throw CommandError("Not a declaration or a command: '"+line+"'");
	}
}

void Main::configure_impl() {

}

void Main::execute_request_impl(xeus::xrequest_context request_context, send_reply_callback cb, int execution_counter, const std::string& code, xeus::execute_request_config , nl::json ) {

	static const boost::regex line_rex("[^\\n\\r]*");
	boost::sregex_iterator line_iter(code.begin(),code.end(),line_rex);
	while(line_iter!=boost::sregex_iterator()){
		if(boost::regex_match(line_iter->str(),boost::regex("\\s*"))){
			line_iter++;
			continue;
		}

		bool was_error = false;
		string error_name;
		string error_msg;
		try{
			consume_line(line_iter->str());
		}
		catch(const NamedError& err){
			was_error=true;
			error_name=err.name;
			error_msg=err.what();
		}
		catch(const std::runtime_error& err){
			was_error=true;
			error_name="std::runtime_error";
			error_msg=err.what();
		}

		string out = std::move(output.str());
		output.str("");
		if(was_error){
			publish_execution_error(request_context, error_name, error_msg, {});
		}
		else{
			nl::json pub_data;
			pub_data["text/plain"] = out;
			publish_execution_result(request_context, execution_counter, std::move(pub_data), nl::json::object());
		}
		line_iter++;
	}
	cb(xeus::create_successful_reply());
}

nl::json Main::complete_request_impl(const std::string& code, int cursor_pos) {

	string target=code;
	target.insert(cursor_pos,"\x1F");
	static const boost::regex cursor_word_rex("\\b\\S*?\\x1F\\S*?\\b");
	boost::smatch rex_results;
	if(boost::regex_search(target,rex_results,cursor_word_rex)){
		string hovered = rex_results.str();
		int cloc = hovered.find("\x1F");
		hovered.erase(cloc,1);
		if(!hovered.empty()){
			std::vector<string> candidates;
			for(const std::pair<string,Expr>& named_expr : workspace){
				if(named_expr.first.size()<=hovered.size())
					continue;
				if(named_expr.first.substr(0,hovered.size())==hovered)
					candidates.push_back(named_expr.first);
			}
			return xeus::create_complete_reply(std::move(candidates),rex_results.position(),
																				 rex_results.position()+rex_results.length());
		}
	}
	return xeus::create_complete_reply({},cursor_pos,cursor_pos);
}

nl::json Main::inspect_request_impl(const std::string& code, int cursor_pos, int ) {
	string target=code;
	target.insert(cursor_pos,"\x1F");
	static const boost::regex cursor_word_rex("\\b\\S*?\\x1F\\S*?\\b");
	boost::smatch rex_results;
	if(boost::regex_search(target,rex_results,cursor_word_rex)){
		string hovered = rex_results.str();
		int cloc = hovered.find("\x1F");
		hovered.erase(cloc,1);
		if(workspace.contains(hovered)){
			return xeus::create_inspect_reply(true,{"text/plain",to_string(workspace[hovered])});
		}
	}
	return xeus::create_inspect_reply();
}

nl::json Main::is_complete_request_impl(const std::string& ) {
	return xeus::create_is_complete_reply("complete");
}

nl::json Main::kernel_info_request_impl() {
	return xeus::create_info_reply(
		"",//protocol_version
		"symbolic",//implementation
		"0.0",//implementation_version
		"symbolic",//language_name
		"0.0",//language_version
		"text/plain",//language_mimetype
		".sym"//language_file_extension
	);
}

void Main::shutdown_request_impl() {

}
