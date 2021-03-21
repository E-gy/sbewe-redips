#include <yasync.hpp>
#include <http/rr.hpp>
#include <util/interproc.hpp>
#include <config/config.hpp>
#include <virt/r1ot.hpp>
#include <virt/statfs.hpp>
#include <virt/bauth.hpp>
#include <fiz/pservr.hpp>
#include <fiz/pservrs.hpp>
#include <ossl/ssl.hpp>

#include <iostream>
#include <unordered_map>
#include <utility>
#include <fstream>
#include <unordered_set>

namespace redips {

// helper type for the visitor
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using namespace redips;
using namespace magikop;

result<config::Config, std::string> parseArgs(int argc, char* args[], bool& dry){
	if(argc < 2) return std::string("No config file given!"); //TODO better args parsing?
	std::string cfgfnam;
	if(argc == 2) cfgfnam = args[1];
	else {
		if(argc > 3) std::cerr << "WARN: ignoring superflous arguments\n";
		if(std::string(args[1]) == "-t") dry = true;
		else return std::string("Unknown switch");
		cfgfnam = args[2];
	}
	std::ifstream cfgf(cfgfnam);
	if(cfgf.fail()) return std::string("Couldn't read config file");
	return config::parse(cfgf);
}

int main(int argc, char* args[]){
	bool dry = false;
	auto par = parseArgs(argc, args, dry);
	if(auto err = par.err()){
		std::cerr << *err << "\n";
		return 1;
	}
	auto config = *par.ok();
	if(dry || config.vhosts.empty()) return 0;
	yasync::io::SystemNetworkingStateControl _sysnet;
	yasync::io::ssl::SSLStateControl _ssl;
	{
		yasync::Yengine engine(4);
		auto timeToStahpRes = yasync::io::CtrlC::on(&engine);
		if(auto err = timeToStahpRes.err()){
			std::cerr << "failed to set up ctrl+c listener: " << *err << "\n";
			return 1;
		}
		auto dun = yasync::aggregAll<void>();
		auto timeToStahp = timeToStahpRes.ok();
		{
			yasync::io::IOYengine ioengine(&engine);
			fiz::ConnectionCare conca;
			std::shared_ptr<std::list<fiz::SListener>> lists(new std::list<fiz::SListener>());
			bool okay = true;
			//--
			std::unordered_map<IPp, virt::R1otBuilder> terms;
			std::unordered_map<IPp, HostMapper<yasync::io::ssl::SharedSSLContext>> sslctx;
			for(auto vhost : config.vhosts){
				virt::SServer stack = std::visit(overloaded {
					[&](const config::FileServer fs){ return virt::SServer(new virt::StaticFileServer(&ioengine, fs.root, fs.defaultFile.value_or("index.html"))); },
					[&](const config::Proxy) -> virt::SServer { throw std::invalid_argument("Proxying not yet implemented!"); }
				}, vhost.mode);
				if(auto auth = vhost.auth) stack = virt::putBehindBasicAuth(auth->realm, auth->credentials, std::move(stack));
				auto fiz = &terms[vhost.address];
				fiz->addService(vhost.tok(), stack);
				if(vhost.isDefault) fiz->setDefaultService(stack);
				if(auto ssl = vhost.tls) yasync::io::ssl::createSSLContext(ssl->cert, ssl->key) >> ([&](auto sctx){ sslctx[vhost.address].addHost(sctx, vhost.tok()); }|[&](auto err){ std::cerr << "Failed to initalize VHost ssl context: " << err << "\n"; });
			}
			for(auto ent : terms){
				if(!okay) break;
				(sslctx.count(ent.first) > 0 ? fiz::listenOnSecure(&ioengine, ent.first, sslctx[ent.first], &conca, ent.second.build()) : fiz::listenOn(&ioengine, ent.first, &conca, ent.second.build())) >> ([&](fiz::SListener li){
					lists->push_back(li);
					dun->add(&engine, li->onShutdown());
				}|[&](auto err){
					std::cerr << "Error launching listener: " << err << "\n";
					okay = false;
				});
			}
			//--
			if(okay){
				engine <<= *timeToStahp >> [lists](){
					std::cout << "Shutting down...\n";
					for(auto li : *lists) li->shutdown();
				};
				std::cout << "listening...\n";
				interproc::extNotifyReady(argc, args);
			} else for(auto li : *lists) li->shutdown();
			yasync::blawait<void>(&engine, dun);
			std::cout << "Closing idle connections\n";
			conca.shutdown();
			ioengine.wioe();
		}
		yasync::io::CtrlC::un(&engine);
		engine.wle();
	}
	return 0;
}

}

int main(int argc, char* args[]){
	return redips::main(argc, args);
}
