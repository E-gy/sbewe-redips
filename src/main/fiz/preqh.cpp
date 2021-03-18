#include "preqh.hpp"

#include <iostream>
#include <ctime>
#include <cstring>
#include <iomanip>

namespace redips::fiz {

using namespace magikop;
using namespace http;

void takeCareOfConnection(yasync::io::IOResource conn, virt::SServer vs){
	conn->engine <<= Request::read(conn) >> ([=](SharedRequest reqwest){
		vs->take(conn, reqwest, [=](auto resp){
			auto wr = conn->writer();
			{
				auto t = std::time(nullptr);
				auto tm = *std::gmtime(&t); //FIXME not thread-safe
				resp.setHeader(Header::Date, std::put_time(&tm, "%a, %d %b %Y %H:%M:%S %Z"));
			}
			resp.setHeader(Header::Host, "Redips");
			resp.setHeader(Header::Connection, "closed");
			resp.write(wr);
			conn->engine <<= wr->eod() >> ([](){}|[](auto err){
				std::cerr << "Failed to respond: " << err << "\n";
			});
		});
	}|[](auto err){ std::cerr << "Read request error " << err.desc << "\n"; });
}

}
