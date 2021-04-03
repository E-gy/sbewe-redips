#include "statfs.hpp"

#include <util/filetype.hpp>
#include <util/strim.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace redips::virt {

using namespace magikop;

StaticFileServer::StaticFileServer(yasync::io::IOYengine* e, std::string r, std::string d, bool g) : engine(e), root(r.length() > 0 && r[r.length()-1] == FPATHSEP ? r.substr(0, r.length()-1) : r), deff(d.length() > 0 && d[0] == FPATHSEP ? d.substr(1) : d), gmd(g) {}

static std::string lifi(const std::string& relp, const std::string& path){
	std::ostringstream ostr;
	ostr
		<< "<!DOCTYPE html>\n"
		<< "<html>\n"
		<< "<head>\n"
		<< "<meta charset=utf-8>\n";
	if(path.length() > 0) ostr << "<title>Index of " << path << "</title>\n";
	else ostr << "<title>Index of " << FPATHSEP << "</title>\n";
	ostr
		<< "</head>\n"
		<< "<body>\n"
		<< "<ul>\n";
	ostr << "<li><a href=\"" << path << FPATHSEP << ".." << "\">" << ".." << "</a></li>\n"; //
	for(const auto& ent : std::filesystem::directory_iterator(relp.length() == 0 ? "/" : relp)) ostr << "<li><a href=\"" << path << FPATHSEP << ent.path().filename().generic_string() << "\">" << ent.path().filename().generic_string() << "</a></li>\n";
	ostr 
		<< "</ul>\n"
		<< "</body>\n"
		<< "</html>";
	return ostr.str();
}

void StaticFileServer::take(const ConnectionInfo&, redips::http::SharedRRaw rraw, RespBack respb){
	rraw->as<http::Request>() >> ([&](http::Request req){
		if(req.method != http::Method::HEAD && req.method != http::Method::GET && req.method != http::Method::POST) return respb(http::Response(http::Status::METHOD_NOT_ALLOWED));
		auto relp = root;
		auto path = req.path.substr(0, std::min(req.path.find('?'), req.path.find('#')));
		if(path.length() > 0 && path[path.length()-1] == FPATHSEP) path = path.substr(0, path.length()-1); //If combined path ends with /, remove it. what if it's a file?
		normalizeURIPath(path);
		relp += path;
		if(relp.length() == 0 || getFileType(relp) == FileType::Directory){ //oh so it is a directory. well then let's get default file in it!
			auto relpdf = relp + FPATHSEP + deff;
			if(gmd) switch(getFileType(relpdf)){
				case FileType::Directory:
					if(path.length() == 0) return respb(http::Response(http::Status::OK, lifi(relpdf, path))); //we're in root, and default file is a directory that exists :| let's list its contents...
					else [[fallthrough]];
				case FileType::NE: return respb(http::Response(http::Status::OK, lifi(relp, path))); //doesn't exist. but we can generate one!
				case FileType::File:
				default:
					break;
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
