#include "statfs.hpp"

#include <util/filetype.hpp>
#include <fstream>
#include <iostream>

namespace redips::virt {

using namespace magikop;

StaticFileServer::StaticFileServer(yasync::io::IOYengine* e, std::string r, std::string d) : engine(e), root(r), deff(d) {}

void StaticFileServer::take(const ConnectionInfo&, redips::http::SharedRRaw rraw, RespBack respb){
	rraw->as<http::Request>() >> ([&](http::Request req){
		if(req.method != http::Method::HEAD && req.method != http::Method::GET && req.method != http::Method::POST) return respb(http::Response(http::Status::METHOD_NOT_ALLOWED));
		auto relp = root;
		if(relp[relp.length()-1] != FPATHSEP) relp += '/';
		auto eop = req.path.find('?');
		relp += eop == std::string::npos ? req.path : req.path.substr(0, eop);
		if(relp[relp.length()-1] == '/') relp += deff;
		else if(getFileType(relp) == FileType::Directory) relp += FPATHSEP + deff;
		if(getFileType(relp) != FileType::File) respb(http::Response(http::Status::NOT_FOUND));
		else {
			yasync::io::fileOpenRead(engine, relp) >> ([=](auto f){
				engine->engine <<= (f->template read<std::vector<char>>()) >> ([=](auto ok){
					respb(http::Response(http::Status::OK, ok));
				}|[=](auto err){
					respb(http::Response(http::Status::INTERNAL_SERVER_ERROR, err));
				});
			}|[=](auto err){
				respb(http::Response(http::Status::INTERNAL_SERVER_ERROR, err));
			});
		}
	}|[&](auto){ respb(http::Response(http::Status::BAD_REQUEST)); });
}

}
