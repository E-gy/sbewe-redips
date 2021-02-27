#include "statfs.hpp"

#include <sys/stat.h>
#include <fstream>
#include <iostream>

using namespace yasync;

namespace redips::virt {

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

void StaticFileServer::take(yasync::io::IOResource conn, redips::http::SharedRequest req){
	auto relp = root;
	if(relp[relp.length()-1] != FPATHSEP) relp += '/';
	auto eop = req->path.find('?');
	relp += eop == std::string::npos ? req->path : req->path.substr(0, eop);
	if(relp[relp.length()-1] == '/') relp += deff;
	else if(getFileType(relp) == FileType::Directory) relp += FPATHSEP + deff;
	auto resp = http::SharedResponse(new http::Response());
	auto immwr = [=](){
		auto wr = conn->writer();
		resp->write(wr);
		return wr->eod() >> ([](){}|[](auto err){
			std::cerr << "Failed to respond: " << err << "\n";
		});
	};
	if(getFileType(relp) != FileType::File){
		resp->status = http::Status::NOT_FOUND;
		resp->setBody("File not found :(");
		conn->engine->engine <<= immwr();
	} else {
		yasync::io::fileOpenRead(conn->engine, relp).ifElse([=](auto f) mutable {
			conn->engine->engine <<= (f->template read<std::vector<char>>()) >> ([=](auto ok){
				resp->status = http::Status::OK;
				resp->setBody(ok);
				return immwr();
			}|[=](auto err){
				resp->status = http::Status::INTERNAL_SERVER_ERROR;
				resp->setBody(err);
				return immwr();
			});
		}, [=](auto err){
			resp->status = http::Status::INTERNAL_SERVER_ERROR;
			resp->setBody(err);
			conn->engine->engine <<= immwr();
		});
	}
}

}
