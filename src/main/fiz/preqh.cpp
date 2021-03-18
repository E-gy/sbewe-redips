#include "preqh.hpp"

#include <iostream>

namespace redips::fiz {

using namespace magikop;

void takeCareOfConnection(yasync::io::IOResource conn, virt::SServer vs){
	conn->engine <<= http::Request::read(conn) >> ([=](http::SharedRequest reqwest){
		vs->take(conn, reqwest, [=](auto resp){
			auto wr = conn->writer();
			resp.write(wr);
			conn->engine <<= wr->eod() >> ([](){}|[](auto err){
				std::cerr << "Failed to respond: " << err << "\n";
			});
		});
	}|[](auto err){ std::cerr << "Read request error " << err.desc << "\n"; });
}

}
