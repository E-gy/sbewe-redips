#include "rr.hpp"

#include <algorithm>
#include <cstring>

namespace redips::http {

Request::Request(Method m, std::string p) : method(m), path(p) {}
Response::Response(Status s) : status(s) {}

bool RR::hasHeader(const std::string& h){ return headers.count(h) > 0; }
bool RR::hasHeader(Header h){ return hasHeader(headerGetStr(h)); }
std::optional<std::string> RR::getHeader(const std::string& h){ return hasHeader(h) ? std::optional(headers[h]) : std::nullopt; }
std::optional<std::string> RR::getHeader(Header h){ return getHeader(headerGetStr(h)); }
void RR::setHeader(const std::string& h, const std::string& v){ headers[h] = v; }
void RR::setHeader(Header h, const std::string& v){ return setHeader(headerGetStr(h), v); }
void RR::removeHeader(const std::string& h){ headers.erase(h); }
void RR::removeHeader(Header h){ return removeHeader(headerGetStr(h)); }

template<> void RR::setBody<std::vector<char>>(const std::vector<char>& data){
	body = data;
}

void RR::setBody(const char* data){
	body = std::vector<char>(data, data+::strlen(data));
}

yasync::Future<RRReadResult> RR::read(SharedRR rr, yasync::io::IOResource res){
	return res->read<std::string>(std::string(CRLF)) >> [=](auto rres){
		if(auto err = rres.err()) return yasync::completed(RRReadResult::Err(RRReadError(*err)));
		auto pr = rr->readTitle(*rres.ok());
		if(pr.isErr()) return yasync::completed(std::move(pr));
		return res->read<std::string>(std::string("\r\n\r\n")) >> [=](auto rres){
			if(auto err = rres.err()) return yasync::completed(RRReadResult::Err(RRReadError(*err)));
			auto pr = rr->readHeaders(*rres.ok());
			if(pr.isErr()) return yasync::completed(std::move(pr));
			auto bls = rr->getHeader(Header::ContentLength);
			std::size_t bl = bls.has_value() ? std::stoull(*bls) : 0;
			if(bl == 0) return yasync::completed(RRReadResult::Ok());
			return res->read<std::vector<char>>(bl) >> [=](auto rres){
				if(auto err = rres.err()) return yasync::completed(RRReadResult::Err(RRReadError(*err)));
				rr->body = std::move(*rres.ok());
				return yasync::completed(RRReadResult::Ok());
			};
		};
	};
}

yasync::Future<result<SharedRequest, RRReadError>> Request::read(yasync::io::IOResource res){
	SharedRequest rr(new Request());
	return RR::read(rr, res) >> [rr](auto rres) -> result<SharedRequest, RRReadError> {
		if(auto err = rres.err()) return *err;
		else return rr;
	};
}
yasync::Future<result<SharedResponse, RRReadError>> Response::read(yasync::io::IOResource res){
	SharedResponse rr(new Response());
	return RR::read(rr, res) >> [rr](auto rres) -> result<SharedResponse, RRReadError>{
		if(auto err = rres.err()) return *err;
		else return rr;
	};
}

yasync::Future<result<SharedRRaw, RRReadError>> RRaw::read(yasync::io::IOResource res){
	SharedRRaw rr(new RRaw());
	return RR::read(rr, res) >> [rr](auto rres) -> result<SharedRRaw, RRReadError>{
		if(auto err = rres.err()) return *err;
		else return rr;
	};
}

RRReadResult RR::readHeaders(const std::string& s){
	std::size_t nhs = 0;
	while(nhs < s.length()){
		auto nhe = s.find(CRLF, nhs);
		if(nhe == std::string::npos) return RRReadError(Status::BAD_REQUEST, "Malformed headers");
		if(nhe == nhs) break; //reached body
		auto colsep = s.find(':', nhs);
		if(colsep == std::string::npos || colsep > nhe) return RRReadError(Status::BAD_REQUEST, "Malformed header line");
		std::size_t hns = nhs;
		for(; hns < colsep && std::isspace(s[hns]); hns++);
		if(hns == colsep) return RRReadError(Status::BAD_REQUEST, "Malformed header line");
		std::size_t hne = colsep-1;
		for(; hne > hns && std::isspace(s[hne]); hne--);
		if(hne == hns) return RRReadError(Status::BAD_REQUEST, "Malformed header line"); 
		std::size_t hvs = colsep+1;
		for(; hvs < nhe && std::isspace(s[hvs]); hvs++);
		if(hvs == nhe) return RRReadError(Status::BAD_REQUEST, "Malformed header line");
		setHeader(s.substr(hns, hne+1-hns), s.substr(hvs, nhe-hvs));
		nhs = nhe+2;
	}
	auto clh = getHeader(Header::ContentLength);
	if(clh.has_value()) try {
		std::stoull(*clh);
	} catch(std::invalid_argument&){
		return RRReadError(Status::BAD_REQUEST, "Content length specified but invalid");
	}
	else setHeader(Header::ContentLength, 0);
	return RRReadResult::Ok();
}

void RR::writeHeaders(std::ostream& w) const {
	w << headerGetStr(Header::ContentLength) << HSEP << (body.has_value() ? body.value().size() : 0) << CRLF;
	for(auto h : headers) if(h.first != headerGetStr(Header::ContentLength)) w << h.first << HSEP << h.second << CRLF;
}

RRReadResult Request::readTitle(const std::string& l){
	if(std::count(l.begin(), l.end(), SP) != 2) return RRReadError(Status::BAD_REQUEST, "Malformed title");
	std::istringstream s(l);
	std::string ms, ps, vs;
	s >> ms >> ps >> vs;
	if(auto v = versionFromStr(vs))
		if(versionIsSupported(*v)) version = *v;
		else return RRReadError(*v == http::Version::HTTP10 || *v == http::Version::HTTP09 ? Status::UPGRADE_REQUIRED : Status::HTTP_VERSION_NOT_SUPPORTED, "Unsupported HTTP version");
	else return RRReadError(Status::BAD_REQUEST, "Not an HTTP");
	if(auto m = methodFromStr(ms)) method = *m;
	else return RRReadError(Status::BAD_REQUEST, "Invalid HTTP method");
	path = ps;
	return RRReadResult::Ok();
}

RRReadResult Response::readTitle(const std::string& l){
	if(std::count(l.begin(), l.end(), SP) < 2) return RRReadError(Status::BAD_REQUEST, "Malformed title");
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

void RR::write(const yasync::io::IORWriter& w) const {
	std::ostringstream os;
	writeTitle(os);
	writeHeaders(os);
	w << os << CRLF;
	if(body.has_value()) w->write(body.value());
}

void Request::writeTitle(std::ostream& w) const {
	w << methodGetStr(method) << SP << path << SP << versionGetStr(version) << CRLF;
}
void Response::writeTitle(std::ostream& w) const {
	w << versionGetStr(version) << SP << statusGetCode(status) << SP << statusGetMessage(status) << CRLF;
}

RRaw::RRaw(const std::string& t) : title(t){}
RRaw::RRaw(const RR& r) : RR(r) {
	std::ostringstream os;
	r.writeTitle(os);
	title = os.str();
}
RRReadResult RRaw::readTitle(const std::string& t){
	title = t;
	return RRReadResult::Ok();
}
void RRaw::writeTitle(std::ostream& os) const {
	os << title;
}

}
