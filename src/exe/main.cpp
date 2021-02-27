#include <sample.hpp>

#include <yasync.hpp>
#include <http/rr.hpp>
#include <util/interproc.hpp>
#include <config/config.hpp>
#include <virt/r1ot.hpp>
#include <virt/statfs.hpp>
#include <fiz/pservr.hpp>

#include <iostream>
#include <map>
#include <utility>
#include <fstream>

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
			for(auto vhost : config.vhosts) terms[{vhost.ip, vhost.port}].addService(vhost.serverName, virt::SServer(new virt::StaticFileServer(vhost.root, vhost.defaultFile.value_or("index.html"))));
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
}
