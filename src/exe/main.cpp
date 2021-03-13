#include <yasync.hpp>
#include <http/rr.hpp>
#include <util/interproc.hpp>
#include <config/config.hpp>
#include <virt/r1ot.hpp>
#include <virt/statfs.hpp>
#include <virt/bauth.hpp>
#include <fiz/pservr.hpp>
#include <ossl/ssl.hpp>

#include <iostream>
#include <map>
#include <utility>
#include <fstream>
#include <unordered_set>

namespace redips {

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

std::unordered_set<std::string> genHostMappings(const config::VHost& vh){
	std::unordered_set<std::string> w;
	w.insert(vh.serverName);
	w.insert(vh.ip);
	if(vh.ip == "localhost" || vh.ip == "0.0.0.0" || vh.ip == "127.0.0.1"){
		w.insert("localhost");
		w.insert("0.0.0.0");
		w.insert("127.0.0.1");
		//TODO moar wildcardz
	}
	auto port = std::to_string(vh.port);
	std::unordered_set<std::string> m;
	for(auto s : w){
		m.insert(s);
		m.insert(s + ":" + port);
	}
	return m;
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
	ssl::SSLStateControl _ssl;
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
			std::shared_ptr<std::list<fiz::SListener>> lists(new std::list<fiz::SListener>());
			bool okay = true;
			//--
			// subject to changes!
			//TODO cross-serve smh
			std::map<std::pair<std::string, unsigned>, virt::R1otBuilder> terms;
			for(auto vhost : config.vhosts){
				auto stack = virt::SServer(new virt::StaticFileServer(vhost.root, vhost.defaultFile.value_or("index.html")));
				if(auto auth = vhost.auth) stack = virt::putBehindBasicAuth(auth->realm, auth->credentials, std::move(stack));
				auto fiz = &terms[{vhost.ip, vhost.port}];
				fiz->addService(vhost.serverName, stack);
				for(auto h : genHostMappings(vhost)) fiz->addService2(h, stack);
				if(vhost.isDefault) fiz->setDefaultService(stack);
			}
			for(auto ent : terms){
				if(!okay) break;
				fiz::listenOn(&ioengine, ent.first.first, ent.first.second, ent.second.build()) >> ([&](fiz::SListener li){
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
		}
		yasync::io::CtrlC::un(&engine);
		engine.wle();
	}
	std::cout << "slaying d~~a~~emons\n";
	Daemons::waitForEveryoneToStop();
	std::this_thread::sleep_for(std::chrono::milliseconds(5)); //HacKeRmANn
	return 0;
}

}

int main(int argc, char* args[]){
	return redips::main(argc, args);
}
