#include "statfs.hpp"

#include <sys/stat.h>
#include <fstream>
#include <iostream>

namespace redips::virt {

using namespace magikop;

StaticFileServer::StaticFileServer(std::string r, std::string d) : root(r), deff(d) {}

constexpr auto FPATHSEP = '/';

enum class FileType {
	NE, Directory, File
};

FileType getFileType(const std::string& f){
	struct ::stat finf;
	if(::stat(f.c_str(), &finf) < 0) return FileType::NE;
	return S_ISDIR(finf.st_mode) ? FileType::Directory : FileType::File;
}

void StaticFileServer::take(yasync::io::IOResource conn, redips::http::SharedRequest req, RespBack respb){
	auto relp = root;
	if(relp[relp.length()-1] != FPATHSEP) relp += '/';
	auto eop = req->path.find('?');
	relp += eop == std::string::npos ? req->path : req->path.substr(0, eop);
	if(relp[relp.length()-1] == '/') relp += deff;
	else if(getFileType(relp) == FileType::Directory) relp += FPATHSEP + deff;
	if(getFileType(relp) != FileType::File) respb(http::Response(http::Status::NOT_FOUND, "File not found :("));
	else {
		yasync::io::fileOpenRead(conn->engine, relp).ifElse([=](auto f) mutable {
			conn->engine->engine <<= (f->template read<std::vector<char>>()) >> ([=](auto ok){
				respb(http::Response(http::Status::OK, ok));
			}|[=](auto err){
				respb(http::Response(http::Status::INTERNAL_SERVER_ERROR, err));
			});
		}, [=](auto err){
			respb(http::Response(http::Status::INTERNAL_SERVER_ERROR, err));
		});
	}
}

}
