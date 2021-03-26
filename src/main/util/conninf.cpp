#include "conninf.hpp"

#include <sstream>

std::ostream& operator<<(std::ostream& os, const ConnectionInfo& conn){
	os << "for=" << conn.address;
	if(conn.host) os << ";host=" << *conn.host;
	return os << ";proto=" << conn.protocol;
}

std::string ConnectionInfo::to_string() const {
	std::ostringstream os;
	os << *this;
	return os.str();
}
