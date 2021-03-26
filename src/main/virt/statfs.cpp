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
		if(relp.length() > 0 && relp[relp.length()-1] == FPATHSEP) relp = relp.substr(0, relp.length()-1); //If root ends with /, remove it as req path is required to start with /
		relp += req.path.substr(0, std::min(req.path.find('?'), req.path.find('#')));
		if(relp.length() > 0 && relp[relp.length()-1] == FPATHSEP) relp = relp.substr(0, relp.length()-1); //If combined path ends with /, remove it. what if it's a file?
		if(getFileType(relp) == FileType::Directory) relp += FPATHSEP + deff; //oh so it is a directory. well then let's get default file in it!
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
	}|[&](auto err){ respb(http::Response(err.asstat.value_or(http::Status::INTERNAL_SERVER_ERROR))); });
}

}
