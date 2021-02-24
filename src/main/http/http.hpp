#pragma once

#include <stdint.h>
#include <string>
#include <optional>

namespace redips::http {

constexpr auto SP = ' ';
constexpr auto CRLF = "\r\n";
constexpr auto HSEP = ": ";

enum class Version {
	HTTP09, HTTP10, HTTP11, HTTP20, HTTP30
};

inline bool versionIsSupported(Version v){
	return v == Version::HTTP11;
}

inline constexpr const char* versionGetStr(Version v){
	switch(v){
		case Version::HTTP09: return "HTTP/0.9";
		case Version::HTTP10: return "HTTP/1.0";
		case Version::HTTP11: return "HTTP/1.1";
		case Version::HTTP20: return "HTTP/2.0";
		case Version::HTTP30: return "HTTP/3.0";
		default: return "UNK";
	}
}

std::optional<Version> versionFromStr(const std::string& s){
	if(s == "HTTP/0.9") return Version::HTTP09;
	if(s == "HTTP/1.0") return Version::HTTP10;
	if(s == "HTTP/1.1") return Version::HTTP11;
	if(s == "HTTP/2.0") return Version::HTTP20;
	if(s == "HTTP/3.0") return Version::HTTP30;
	return std::nullopt;
}

enum class Method {
	GET, HEAD, POST, PUT, DELETE, CONNECT, OPTIONS, TRACE, PATCH
};

inline bool methodIsSupported(Method m){
	switch(m){
		case Method::GET:
		case Method::HEAD:
		case Method::POST:
			return true;
		default: return false;
	}
}

inline constexpr const char* methodGetStr(Method m){
	switch(m){
		case Method::GET: return "GET";
		case Method::HEAD: return "HEAD";
		case Method::POST: return "POST";
		case Method::PUT: return "PUT";
		case Method::DELETE: return "DELETE";
		case Method::CONNECT: return "CONNECT";
		case Method::OPTIONS: return "OPTIONS";
		case Method::TRACE: return "TRACE";
		case Method::PATCH: return "PATCH";
		default: return "UNK";
	}
}

std::optional<Method> methodFromStr(const std::string& s){
	if(s == "GET") return Method::GET;
	if(s == "HEAD") return Method::HEAD;
	if(s == "POST") return Method::POST;
	if(s == "PUT") return Method::PUT;
	if(s == "DELETE") return Method::DELETE;
	if(s == "CONNECT") return Method::CONNECT;
	if(s == "OPTIONS") return Method::OPTIONS;
	if(s == "TRACE") return Method::TRACE;
	if(s == "PATCH") return Method::PATCH;
	return std::nullopt;
}

enum class Status : uint16_t {
	OK = 200,
	BAD_REQUEST = 400,
	UNAUTHORIZED = 401,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
	METHOD_NOT_ALLOWED = 405,
	PROXY_AUTHENTICATION_REQUIRED = 407,
	REQUEST_TIMEOUT = 408,
	PAYLOAD_TOO_LARGE = 413,
	URI_TOO_LONG = 414,
	UPGRADE_REQUIRED = 426,
	HEADER_FIELDS_TOO_LARGE = 431,
	INTERNAL_SERVER_ERROR = 500,
	NOT_IMPLEMENTED = 501,
	BAD_GATEWAY = 502,
	SERVICE_UNAVAILABLE = 503,
	GATEWAY_TIMEOUT = 504,
	HTTP_VERSION_NOT_SUPPORTED = 505
};

inline constexpr uint16_t statusGetCode(Status s){
	return static_cast<uint16_t>(s);
}

template<typename Int> std::optional<Status> statusFromCode(Int code){
	auto s = static_cast<Status>(code);
	switch(s){
		case Status::OK:
		case Status::BAD_REQUEST:
		case Status::UNAUTHORIZED:
		case Status::FORBIDDEN:
		case Status::NOT_FOUND:
		case Status::METHOD_NOT_ALLOWED:
		case Status::PROXY_AUTHENTICATION_REQUIRED:
		case Status::REQUEST_TIMEOUT:
		case Status::PAYLOAD_TOO_LARGE:
		case Status::URI_TOO_LONG:
		case Status::UPGRADE_REQUIRED:
		case Status::HEADER_FIELDS_TOO_LARGE:
		case Status::INTERNAL_SERVER_ERROR:
		case Status::NOT_IMPLEMENTED:
		case Status::BAD_GATEWAY:
		case Status::SERVICE_UNAVAILABLE:
		case Status::GATEWAY_TIMEOUT:
		case Status::HTTP_VERSION_NOT_SUPPORTED:
			return s;
		default: return std::nullopt;
	}
}

/**
 * Get human readable message for HTTP status
 * @param status http status
 * @returns @const string human readabale description for the status
 */
inline constexpr const char* statusGetMessage(Status status){
	switch(status){
		case Status::OK: return "OK";
		case Status::BAD_REQUEST: return "Bad Request";
		case Status::UNAUTHORIZED: return "Unauthorized";
		case Status::FORBIDDEN: return "Forbidden";
		case Status::NOT_FOUND: return "Not Found";
		case Status::METHOD_NOT_ALLOWED: return "Method Not Allowed";
		case Status::PROXY_AUTHENTICATION_REQUIRED: return "Proxy Authentication Required";
		case Status::REQUEST_TIMEOUT: return "Request Timeout";
		case Status::PAYLOAD_TOO_LARGE: return "Payload Too Large";
		case Status::URI_TOO_LONG: return "URI Too Long";
		case Status::UPGRADE_REQUIRED: return "Upgrade Required";
		case Status::HEADER_FIELDS_TOO_LARGE: return "Request Header Fields Too Large";
		case Status::INTERNAL_SERVER_ERROR: return "Internal Server Error";
		case Status::NOT_IMPLEMENTED: return "Not Implemented";
		case Status::BAD_GATEWAY: return "Bad Gateway";
		case Status::SERVICE_UNAVAILABLE: return "Service Unavailable";
		case Status::GATEWAY_TIMEOUT: return "Gateway Timeout";
		case Status::HTTP_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
		default: return "Private use status";
	}
}

enum class Header {
	Host, ContentLength, Date, Connection
};

inline constexpr const char* headerGetStr(Header h){
	switch(h){
		case Header::Host: return "Host";
		case Header::ContentLength: return "Content-Length";
		case Header::Date: return "Date";
		case Header::Connection: return "Connection";
		default: return "";
	}
}

}
