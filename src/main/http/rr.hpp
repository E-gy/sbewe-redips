#pragma once

#include "http.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <yasync/io.hpp>

namespace redips::http {

struct Request;
struct Response;
struct RRaw;
struct RR;
using SharedRequest = std::shared_ptr<Request>;
using SharedResponse = std::shared_ptr<Response>;
using SharedRRaw = std::shared_ptr<RRaw>;
using SharedRR = std::shared_ptr<RR>;

struct RRReadError {
	std::optional<Status> asstat;
	std::string desc;
	RRReadError(std::string d): asstat(std::nullopt), desc(d) {}
	RRReadError(Status s, std::string d): asstat(s), desc(d) {}
};
using RRReadResult = result<std::size_t, RRReadError>;

struct RR {
	Version version = http::Version::HTTP11;
	std::unordered_map<std::string, std::string> headers;
	std::optional<std::vector<char>> body; //TODO byte it up
	RR() = default;
	bool hasHeader(const std::string& h);
	bool hasHeader(Header h);
	std::optional<std::string> getHeader(const std::string& h);
	std::optional<std::string> getHeader(Header h);
	void setHeader(const std::string& h, const std::string& v);
	void setHeader(Header h, const std::string& v);
	template<typename V> void setHeader(const std::string& h, const V& v){
		std::ostringstream s;
		s << v;
		return setHeader(h, s.str());
	}
	template<typename V> void setHeader(Header h, const V& v){ return setHeader(headerGetStr(h), v); }
	void removeHeader(const std::string& h);
	void removeHeader(Header h);
	//body
	bool hasBody();
	void removeBody();
	template<typename Iter> void setBody(const Iter& s, const Iter& e){ body = std::vector<char>(s, e); }
	template<typename T> void setBody(const T& dr){ setBody(dr.begin(), dr.end()); }
	void setBody(const char*);
	//IO
	virtual RRReadResult readTitle(const std::string&) = 0;
	virtual void writeTitle(std::ostream&) const = 0;
	RRReadResult readHeaders(const std::string&);
	void writeHeaders(std::ostream&) const;
	void write(const yasync::io::IORWriter&) const;
	static yasync::Future<RRReadResult> read(SharedRR, yasync::io::IOResource);
};

template<> void RR::setBody<std::vector<char>>(const std::vector<char>& data);

struct Request : public RR {
	Method method;
	std::string path;
	Request() = default;
	Request(Method, std::string path);
	template<typename T> Request(Method m, std::string path, const T& body) : Request(m, path) { setBody(body); }
	static yasync::Future<result<SharedRequest, RRReadError>> read(yasync::io::IOResource);
	RRReadResult readTitle(const std::string&) override;
	void writeTitle(std::ostream&) const override;
};

struct Response : public RR {
	Status status;
	Response() = default;
	explicit Response(Status);
	template<typename T> Response(Status s, const T& body) : Response(s) { setBody(body); }
	static yasync::Future<result<SharedResponse, RRReadError>> read(yasync::io::IOResource);
	RRReadResult readTitle(const std::string&) override;
	void writeTitle(std::ostream&) const override;
};

struct RRaw : public RR {
	std::string title;
	RRaw() = default;
	RRaw(const std::string&);
	RRaw(const RR&);
	template<typename R> result<R, RRReadError> as(){
		R r;
		if(auto err = r.readTitle(title).errOpt()) return result<R, RRReadError>::Err(*err);
		r.headers = headers;
		r.body = body;
		return r;
	}
	static yasync::Future<result<SharedRRaw, RRReadError>> read(yasync::io::IOResource);
	RRReadResult readTitle(const std::string&) override;
	void writeTitle(std::ostream&) const override;
};

}
