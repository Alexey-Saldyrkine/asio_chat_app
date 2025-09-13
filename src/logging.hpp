#pragma once
#include <iostream>
#include <boost/system.hpp>
using boostError = boost::system::error_code;

template<typename ... Args>
void logError(Args &&... args) {
	//(std::cerr<<...<<args) << "\n";
}

template<typename ... Args>
void logAction(Args &&... args) {
//	(std::cout<<...<<args) << "\n";
}

std::runtime_error prependError(const char *where, const boostError &er) {
std::string str(where);
str += ": " + er.what();
return std::runtime_error(str);
}
