#pragma once

#include <stdint.h>

namespace redips::http {

constexpr auto SP = " ";
constexpr auto CRLF = "\r\n";
constexpr auto HSEP = ": ";

enum class Version {
	HTTP09, HTTP10, HTTP11, HTTP20, HTTP30
};

inline bool versionIsSupported(Version v){
	return v == Version::HTTP11;
}

inline constexpr decltype(CRLF) versionGetStr(Version v){
	switch(v){
		case Version::HTTP09: return "HTTP/0.9";
		case Version::HTTP10: return "HTTP/1.0";
		case Version::HTTP11: return "HTTP/1.1";
		case Version::HTTP20: return "HTTP/2.0";
		case Version::HTTP30: return "HTTP/3.0";
		default: return "UNK";
	}
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

inline constexpr decltype(CRLF) methodGetStr(Method m){
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

/**
 * Get human readable message for HTTP status
 * @param status http status
 * @returns @const string human readabale description for the status
 */
inline constexpr decltype(CRLF) statusGetMessage(Status status){
	switch(status){
		case Status::OK: "OK";
		case Status::BAD_REQUEST: "Bad Request";
		case Status::UNAUTHORIZED: "Unauthorized";
		case Status::FORBIDDEN: "Forbidden";
		case Status::NOT_FOUND: "Not Found";
		case Status::METHOD_NOT_ALLOWED: "Method Not Allowed";
		case Status::PROXY_AUTHENTICATION_REQUIRED: "Proxy Authentication Required";
		case Status::REQUEST_TIMEOUT: "Request Timeout";
		case Status::PAYLOAD_TOO_LARGE: "Payload Too Large";
		case Status::URI_TOO_LONG: "URI Too Long";
		case Status::UPGRADE_REQUIRED: "Upgrade Required";
		case Status::HEADER_FIELDS_TOO_LARGE: "Request Header Fields Too Large";
		case Status::INTERNAL_SERVER_ERROR: "Internal Server Error";
		case Status::NOT_IMPLEMENTED: "Not Implemented";
		case Status::BAD_GATEWAY: "Bad Gateway";
		case Status::SERVICE_UNAVAILABLE: "Service Unavailable";
		case Status::GATEWAY_TIMEOUT: "Gateway Timeout";
		case Status::HTTP_VERSION_NOT_SUPPORTED: "HTTP Version Not Supported";
		default: return "Private use status";
	}
}

enum class Header {
	Host, ContentLength, Date, Connection
};

inline constexpr decltype(CRLF) headerGetStr(Header h){
	switch(h){
		case Header::Host: return "Host";
		case Header::ContentLength: return "Content-Length";
		case Header::Date: return "Date";
		case Header::Connection: return "Connection";
		default: return "";
	}
}

}
