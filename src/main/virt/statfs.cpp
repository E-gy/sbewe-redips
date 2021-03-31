#include "statfs.hpp"

#include <util/filetype.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace redips::virt {

using namespace magikop;

StaticFileServer::StaticFileServer(yasync::io::IOYengine* e, std::string r, std::string d, bool g) : engine(e), root(r.length() > 0 && r[r.length()-1] == FPATHSEP ? r.substr(0, r.length()-1) : r), deff(d.length() > 0 && d[0] == FPATHSEP ? d.substr(1) : d), gmd(g) {}

void StaticFileServer::take(const ConnectionInfo&, redips::http::SharedRRaw rraw, RespBack respb){
	rraw->as<http::Request>() >> ([&](http::Request req){
		if(req.method != http::Method::HEAD && req.method != http::Method::GET && req.method != http::Method::POST) return respb(http::Response(http::Status::METHOD_NOT_ALLOWED));
		auto relp = root;
		auto path = req.path.substr(0, std::min(req.path.find('?'), req.path.find('#')));
		if(path.length() > 0 && path[path.length()-1] == FPATHSEP) path = path.substr(0, path.length()-1); //If combined path ends with /, remove it. what if it's a file?
		relp += path;
		if(relp.length() == 0 || getFileType(relp) == FileType::Directory){ //oh so it is a directory. well then let's get default file in it!
			auto relpdf = relp + FPATHSEP + deff;
			if(gmd && getFileType(relpdf) != FileType::File){ //doesn't exist. but we can generate one!
				std::ostringstream ostr;
				ostr
					<< "<!DOCTYPE html>\r\n"
					<< "<html>\r\n"
					<< "<head>\r\n"
					<< "<meta charset=utf-8>\r\n";
				if(path.length() > 0) ostr << "<title>Index of " << path << "</title>\r\n";
				else ostr << "<title>Index of " << FPATHSEP << "</title>\r\n";
				ostr
					<< "</head>\r\n"
					<< "<body>\r\n"
					<< "<ul>\r\n";
				ostr << "<a href=\"" << path << FPATHSEP << ".." << "\">" << ".." << "</a>\r\n"; //
				for(const auto& ent : std::filesystem::directory_iterator(relp.length() == 0 ? "/" : relp)) ostr << "<a href=\"" << path << FPATHSEP << ent.path().filename().generic_string() << "\">" << ent.path().filename().generic_string() << "</a>\r\n";
				ostr 
					<< "</ul>\r\n"
					<< "</body>\r\n"
					<< "</html>";
				return respb(http::Response(http::Status::OK, ostr.str()));
			}
			relp = relpdf;
		}
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
