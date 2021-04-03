#include "conninf.hpp"

#include <sstream>
#include "ip.hpp"

std::ostream& operator<<(std::ostream& os, const ConnectionInfo& conn){
	os << "for=";
	if(isValidIPv6(conn.address)) os << "\"[" << conn.address << "]\"";
	else os << conn.address;
	if(conn.host){
		os << ";host=";
		if(conn.host->find(':') != conn.host->npos) os << "\"" << *conn.host << "\"";
		else os << *conn.host;
	}
	return os << ";proto=" << conn.protocol;
}

std::string ConnectionInfo::to_string() const {
	std::ostringstream os;
	os << *this;
	return os.str();
}
