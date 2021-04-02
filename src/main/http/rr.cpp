#include "rr.hpp"

#include <algorithm>
#include <cstring>
#include <util/strim.hpp>

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
		const auto bytesT = *pr.ok();
		return res->read<std::string>(std::string("\r\n\r\n")) >> [=](auto rres){
			if(auto err = rres.err()) return yasync::completed(RRReadResult::Err(RRReadError(*err)));
			auto pr = rr->readHeaders(*rres.ok());
			if(pr.isErr()) return yasync::completed(std::move(pr));
			const auto bytesH = *pr.ok();
			auto bls = rr->getHeader(Header::ContentLength);
			size_t ble = 0;
			std::size_t bl = bls.has_value() ? std::stoull(*bls, &ble) : 0;
			if(bls.has_value() && ble == 0) return yasync::completed(RRReadResult::Err(RRReadError(Status::BAD_REQUEST, "Invalid Content-Length")));
			if(bl == 0) return yasync::completed(RRReadResult::Ok(bytesT+bytesH));
			return res->read<std::vector<char>>(bl) >> [=](auto rres){
				if(auto err = rres.err()) return yasync::completed(RRReadResult::Err(RRReadError(*err)));
				const auto bytesB = rres.ok()->size();
				rr->body = std::move(*rres.ok());
				return yasync::completed(RRReadResult::Ok(bytesT+bytesH+bytesB));
			};
		};
	};
}

yasync::Future<result<SharedRequest, RRReadError>> Request::read(yasync::io::IOResource res){
	SharedRequest rr(new Request());
	return RR::read(rr, res) >> [rr](auto rres) -> result<SharedRequest, RRReadError> {
		if(auto err = rres.err()) return *err;
		rr->diagRTotalBytes = *rres.ok();
		return rr;
	};
}
yasync::Future<result<SharedResponse, RRReadError>> Response::read(yasync::io::IOResource res){
	SharedResponse rr(new Response());
	return RR::read(rr, res) >> [rr](auto rres) -> result<SharedResponse, RRReadError>{
		if(auto err = rres.err()) return *err;
		rr->diagRTotalBytes = *rres.ok();
		return rr;
	};
}

yasync::Future<result<SharedRRaw, RRReadError>> RRaw::read(yasync::io::IOResource res){
	SharedRRaw rr(new RRaw());
	return RR::read(rr, res) >> [rr](auto rres) -> result<SharedRRaw, RRReadError>{
		if(auto err = rres.err()) return *err;
		rr->diagRTotalBytes = *rres.ok();
		return rr;
	};
}

RRReadResult RR::readHeaders(const std::string& s){
	std::size_t nhs = 0;
	while(nhs < s.length()){
		auto nhe = s.find(CRLF, nhs);
		if(nhe == std::string::npos) return RRReadError(Status::BAD_REQUEST, "Malformed headers");
		if(nhe == nhs) break; //reached body
		auto colsep = s.find(':', nhs);
		if(colsep == std::string::npos || colsep > nhe || colsep == nhs) return RRReadError(Status::BAD_REQUEST, "Malformed header line: no name:value separation");
		auto hname = s.substr(nhs, colsep-nhs);
		for(auto c : hname) if(std::isspace(c)) return RRReadError(Status::BAD_REQUEST, "Malformed header line: header name must not contain whitespace");
		std::size_t hvs = colsep+1;
		for(; hvs < nhe && std::isspace(s[hvs]); hvs++);
		std::size_t hve = nhe;
		for(; hve > hvs && std::isspace(s[hve]); hve--);
		setHeader(hname, hvs < nhe ? s.substr(hvs, hve+1-hvs) : ""); //https://bugzilla.mozilla.org/show_bug.cgi?id=474845
		nhs = nhe+2;
	}
	if(auto clh = getHeader(Header::ContentLength)) try {
		if(clh->length() == 0) return RRReadError(Status::BAD_REQUEST, "Content length specified but empty");
		if((*clh)[0] == '-') return RRReadError(Status::BAD_REQUEST, "Content length specified negative");
		std::stoull(*clh);
	} catch(std::invalid_argument&){
		return RRReadError(Status::BAD_REQUEST, "Content length specified NaN");
	}
	else setHeader(Header::ContentLength, 0);
	return RRReadResult::Ok(s.length());
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
	else return RRReadError(Status::METHOD_NOT_ALLOWED, "Invalid HTTP method");
	if(ps.length() == 0) path = "/";
	else if(ps[0] == '/') path = ps;
	else if(beginsWith(ps, "http")){
		auto slashslash = ps.find("://");
		if(slashslash == ps.npos) return RRReadError(Status::BAD_REQUEST, "HTTP full URI without protocol delimiter / request path must start with slash");
		auto fslash = ps.find('/', slashslash+3);
		if(fslash == ps.npos) path = "/";
		else path = ps.substr(fslash);
	} else return RRReadError(Status::BAD_REQUEST, "HTTP request path must start with slash");
	path = ps;
	return RRReadResult::Ok(l.length());
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
	return RRReadResult::Ok(l.length());
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
	return RRReadResult::Ok(t.length());
}
void RRaw::writeTitle(std::ostream& os) const {
	os << title;
}

}
