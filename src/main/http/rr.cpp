#include "rr.hpp"

#include <ctime>
#include <iomanip>
#include <algorithm>

namespace redips::http {

bool RR::hasHeader(const std::string& h){ return headers.count(h) > 0; }
bool RR::hasHeader(Header h){ return hasHeader(headerGetStr(h)); }
std::optional<std::string> RR::getHeader(const std::string& h){ return hasHeader(h) ? std::optional(headers[h]) : std::nullopt; }
std::optional<std::string> RR::getHeader(Header h){ return getHeader(headerGetStr(h)); }
void RR::setHeader(const std::string& h, const std::string& v){ headers[h] = v; }
void RR::setHeader(Header h, const std::string& v){ return setHeader(headerGetStr(h), v); }
void RR::removeHeader(const std::string& h){ headers.erase(h); }
void RR::removeHeader(Header h){ return removeHeader(headerGetStr(h)); }

RRReadResult RR::readHeaders(const std::string& s){
	std::size_t nhs = 0;
	while(nhs < s.length()){
		auto nhe = s.find(CRLF, nhs);
		if(nhe == std::string::npos) return RRReadError(Status::BAD_REQUEST, "Malformed headers");
		auto colsep = s.find(":", nhs, nhe-nhs);
		if(colsep == std::string::npos) return RRReadError(Status::BAD_REQUEST, "Malformed header line");
		std::size_t hns = nhs;
		for(; hns < colsep && std::isspace(s[hns]); hns++);
		if(hns == colsep) return RRReadError(Status::BAD_REQUEST, "Malformed header line");
		std::size_t hne = colsep-1;
		for(; hne > hns && std::isspace(s[hne]); hne--);
		if(hne == hns) return RRReadError(Status::BAD_REQUEST, "Malformed header line"); 
		std::size_t hvs = colsep+1;
		for(; hvs < nhe && std::isspace(s[hvs]); hvs++);
		if(hvs == nhe) return RRReadError(Status::BAD_REQUEST, "Malformed header line");
		setHeader(s.substr(hns, hne-hns), s.substr(hvs, nhe));
		nhs = nhe+2;
	}
	return RRReadResult::Ok();
}

RRReadResult Request::readTitle(const std::string& l){
	if(std::count(l.begin(), l.end(), SP) != 3) return RRReadError(Status::BAD_REQUEST, "Malformed title");
	std::istringstream s(l);
	std::string ms, ps, vs;
	s >> ms >> ps >> vs;
	if(auto v = versionFromStr(vs))
		if(versionIsSupported(*v)) version = *v;
		else return RRReadError(Status::HTTP_VERSION_NOT_SUPPORTED, "Unsupported HTTP version");
	else return RRReadError(Status::BAD_REQUEST, "Not an HTTP");
	if(auto m = methodFromStr(ms))
		if(methodIsSupported(*m)) method = *m;
		else return RRReadError(Status::METHOD_NOT_ALLOWED, "Unsupported HTTP method");
	else return RRReadError(Status::BAD_REQUEST, "Invalid HTTP method");
	path = ps;
	return RRReadResult::Ok();
}

RRReadResult Response::readTitle(const std::string& l){
	if(std::count(l.begin(), l.end(), SP) != 3) return RRReadError(Status::BAD_REQUEST, "Malformed title");
	std::istringstream s(l);
	std::string vs, scs, sms;
	s >> vs >> scs >> sms;
	uint16_t si;
	try{
		si = std::stoi(scs);
	} catch(std::invalid_argument&){
		return RRReadError(Status::BAD_REQUEST, "Invalid status code");
	}
	if(auto v = versionFromStr(vs))
		if(versionIsSupported(*v)) version = *v;
		else return RRReadError(Status::HTTP_VERSION_NOT_SUPPORTED, "Unsupported HTTP version");
	else return RRReadError(Status::BAD_REQUEST, "Not an HTTP");
	if(auto sc = statusFromCode(si)) status = *sc;
	else return RRReadError(Status::BAD_REQUEST, "Unsupported status code");
	return RRReadResult::Ok();
}

void RR::write(const yasync::io::IORWriter& w){
	writeTitle(w);
	writeFixHeaders();
	for(auto h : headers) w << h.first << HSEP << h.second << CRLF;
	w << CRLF;
	if(body.has_value()) w->write(body.value());
}

void Request::writeTitle(const yasync::io::IORWriter& w){
	w << methodGetStr(method) << SP << path << SP << versionGetStr(version) << CRLF;
}
void Response::writeTitle(const yasync::io::IORWriter& w){
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
