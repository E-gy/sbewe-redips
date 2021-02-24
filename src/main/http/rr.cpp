#include "rr.hpp"

#include <ctime>
#include <iomanip>

namespace redips::http {

bool RR::hasHeader(const std::string& h){ return headers.count(h) > 0; }
bool RR::hasHeader(Header h){ return hasHeader(headerGetStr(h)); }
std::optional<std::string> RR::getHeader(const std::string& h){ return hasHeader(h) ? std::optional(headers[h]) : std::nullopt; }
std::optional<std::string> RR::getHeader(Header h){ return getHeader(headerGetStr(h)); }
void RR::setHeader(const std::string& h, const std::string& v){ headers[h] = v; }
void RR::setHeader(Header h, const std::string& v){ return setHeader(headerGetStr(h), v); }
void RR::removeHeader(const std::string& h){ headers.erase(h); }
void RR::removeHeader(Header h){ return removeHeader(headerGetStr(h)); }

void RR::write(const yasync::io::IORWriter& w){
	writeFirstLine(w);
	writeFixHeaders();
	for(auto h : headers) w << h.first << HSEP << h.second << CRLF;
	w << CRLF;
	if(body.has_value()) w->write(body.value());
}

void Request::writeFirstLine(const yasync::io::IORWriter& w){
	w << methodGetStr(method) << SP << path << SP << versionGetStr(version) << CRLF;
}
void Response::writeFirstLine(const yasync::io::IORWriter& w){
	w << versionGetStr(version) << SP << statusGetCode(status) << SP << statusGetMessage(status) << CRLF;
}

void RR::writeFixHeaders(){
	setHeader(Header::ContentLength, body.has_value() ? body.value().size() : 0);
}
void Response::writeFixHeaders(){
	RR::writeFixHeaders();
	{
		auto t = std::time(nullptr);
		auto tm = *std::gmtime(&t); //FIXME not thread-safe
		setHeader(Header::Date, std::put_time(&tm, "%a, %d %b %Y %H:%M:%S %Z"));
	}
	setHeader(Header::Host, "Redips");
	setHeader(Header::Connection, "closed");
}

}
