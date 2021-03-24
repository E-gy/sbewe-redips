#include "conninf.hpp"

#include <sstream>

std::ostream& operator<<(std::ostream& os, const ConnectionInfo& conn){
	os << "for=" << conn.address << ";proto=" << conn.protocol;
	if(conn.host) os << ";host" << *conn.host;
	return os;
}

std::string ConnectionInfo::to_string() const {
	std::ostringstream os;
	os << *this;
	return os.str();
}
