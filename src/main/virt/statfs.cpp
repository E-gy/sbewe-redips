#include "statfs.hpp"

#include <util/filetype.hpp>
#include <fstream>
#include <iostream>

namespace redips::virt {

using namespace magikop;

StaticFileServer::StaticFileServer(yasync::io::IOYengine* e, std::string r, std::string d) : engine(e), root(r), deff(d) {}

void StaticFileServer::take([[maybe_unused]] yasync::io::IOResource conn, redips::http::SharedRequest req, RespBack respb){
	auto relp = root;
	if(relp[relp.length()-1] != FPATHSEP) relp += '/';
	auto eop = req->path.find('?');
	relp += eop == std::string::npos ? req->path : req->path.substr(0, eop);
	if(relp[relp.length()-1] == '/') relp += deff;
	else if(getFileType(relp) == FileType::Directory) relp += FPATHSEP + deff;
	if(getFileType(relp) != FileType::File) respb(http::Response(http::Status::NOT_FOUND, "File not found :("));
	else {
		yasync::io::fileOpenRead(engine, relp) >> ([=](auto f) mutable {
			engine->engine <<= (f->template read<std::vector<char>>()) >> ([=](auto ok){
				respb(http::Response(http::Status::OK, ok));
			}|[=](auto err){
				respb(http::Response(http::Status::INTERNAL_SERVER_ERROR, err));
			});
		}|[=](auto err){
			respb(http::Response(http::Status::INTERNAL_SERVER_ERROR, err));
		});
	}
}

}
