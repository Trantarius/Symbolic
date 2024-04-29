#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <ctime>
#include <iostream>
using std::to_string;

struct Test{
	inline static std::vector<Test*> all_tests;

	std::string name="Unnamed Test";
	enum{UNKNOWN,PASSED,FAILED} status=UNKNOWN;
	std::string fail_reason;
	void (*fptr)();
	double timeout;
	std::mutex mtx;

	Test(const std::string& name, void (*fptr)(),double timeout=1):
		name(name), fptr(fptr), timeout(timeout){
		all_tests.push_back(this);
	}

	inline static void t_run(Test* t){
		t->mtx.lock();
		try{
			t->fptr();
			t->status = PASSED;
		}
		catch(const std::runtime_error& err){
			t->fail_reason = err.what();
			t->status = FAILED;
		}
		t->mtx.unlock();
	}

	void run() {
		double start_time = (double)clock() / (double)CLOCKS_PER_SEC;
		std::thread td{t_run,this};
		// give the thread a chance to start
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		while(true){
			if(mtx.try_lock()){
				td.join();
				mtx.unlock();
				break;
			}else{
				double now = (double)clock() / (double)CLOCKS_PER_SEC;
				if(now-start_time > timeout){
					std::cout<<"test "<<name<<" has timed out after "<<std::to_string(timeout)<<"s"<<std::endl;
					std::terminate();
				}
				else{
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
			}
		}
		try{
			fptr();
			status = PASSED;
		}
		catch(const std::runtime_error& err){
			fail_reason = err.what();
			status = FAILED;
		}
	}
};

