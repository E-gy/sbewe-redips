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
using SharedRequest = std::shared_ptr<Request>;
using SharedResponse = std::shared_ptr<Response>;

struct RR {
	Version version;
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
	//IO
	void write(const yasync::io::IORWriter&);
	protected:
		virtual void writeTitle(const yasync::io::IORWriter&) = 0;
		virtual void writeFixHeaders();
};

struct Request : public RR {
	Method method;
	std::string path;
	Request() = default;
	protected:
		void writeTitle(const yasync::io::IORWriter&) override;
};

struct Response : public RR {
	Status status;
	Response() = default;
	protected:
		void writeTitle(const yasync::io::IORWriter&) override;
		void writeFixHeaders() override;
};

}
