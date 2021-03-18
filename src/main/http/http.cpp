#include "http.hpp"

namespace redips::http {

std::optional<Version> versionFromStr(const std::string& s){
	if(s == "HTTP/0.9") return Version::HTTP09;
	if(s == "HTTP/1.0") return Version::HTTP10;
	if(s == "HTTP/1.1") return Version::HTTP11;
	if(s == "HTTP/2.0") return Version::HTTP20;
	if(s == "HTTP/3.0") return Version::HTTP30;
	return std::nullopt;
}

std::optional<Method> methodFromStr(const std::string& s){
	if(s == "GET") return Method::GET;
	if(s == "HEAD") return Method::HEAD;
	if(s == "POST") return Method::POST;
	if(s == "PUT") return Method::PUT;
	if(s == "DELETE") return Method::DELET;
	if(s == "CONNECT") return Method::CONNECT;
	if(s == "OPTIONS") return Method::OPTIONS;
	if(s == "TRACE") return Method::TRACE;
	if(s == "PATCH") return Method::PATCH;
	return std::nullopt;
}

}
