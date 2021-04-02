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
#include <virt/headmod.hpp>
#include <fiz/estconn.hpp>
#include <virt/prox.hpp>
#include <virt/loadbalhelth.hpp>

#include <iostream>
#include <unordered_map>
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

int main(int argc, char* args[]){
	yasync::io::ssl::SSLStateControl _ssl;
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
		yasync::Yengine engine(1);
		auto timeToStahpRes = yasync::io::CtrlC::on(&engine);
		if(auto err = timeToStahpRes.err()){
			std::cerr << "failed to set up ctrl+c listener: " << *err << "\n";
			return 1;
		}
		auto dun = yasync::aggregAll<void>();
		auto timeToStahp = timeToStahpRes.ok();
		{
			constexpr auto estconntimeout = std::chrono::milliseconds(5000);
			yasync::io::IOYengine ioengine(&engine);
			yasync::TickTack ticktack;
			fiz::HealthMonitor lbhmon(&ioengine, std::chrono::seconds(12));
			fiz::ConnectionCare conca(&ticktack, config.timings.keepAlive ? std::optional{std::chrono::seconds(*config.timings.keepAlive)} : std::nullopt, config.timings.transaction ? std::optional{std::chrono::seconds(*config.timings.transaction)} : std::nullopt);
			std::shared_ptr<std::list<fiz::SListener>> lists(new std::list<fiz::SListener>());
			bool okay = true;
			//--
			std::unordered_map<std::string, virt::SServer> ups;
			for(auto u : config.upstreams){
				if(!okay) break;
				virt::SServer uv;
				switch(u.second.lbm){
					case LoadBalancingMethod::RoundRobin:{
						auto bal = std::make_shared<virt::RoundRobinBalancer<virt::ProxyConnectionFactory>>();
						for(auto h : u.second.hosts){
							auto hpfr = fiz::findConnectionFactory(h.address, &ioengine, &ticktack, estconntimeout);
							if(auto err = hpfr.err()){
								std::cerr << "Ups host address lookup error: " << err << "\n";
								okay = false;
								break;
							}
							bal->add(std::move(*hpfr.ok()), h.weight.value_or(1));
						}
						uv = virt::proxyTo(&engine, [bal](){ return (*bal)()(); }, u.second.retries.value_or(0));
						break;
					}
					case LoadBalancingMethod::FailOver:{
						auto bal = std::make_shared<virt::FailOverBalancer<virt::ProxyConnectionFactory>>(&lbhmon, true);
						for(auto h : u.second.hosts){
							auto hpfr = fiz::findConnectionFactory(h.address, &ioengine, &ticktack, estconntimeout);
							if(auto err = hpfr.err()){
								std::cerr << "Ups host address lookup error: " << *err << "\n";
								okay = false;
								break;
							}
							if(auto err = bal->add(std::move(*hpfr.ok()), h.address, *h.healthCheckUrl).errOpt()){
								std::cerr << "Ups host health check init error: " << *err << "\n";
								okay = false;
								break;
							}
						}
						uv = virt::proxyTo(&engine, [bal](){ if(auto alive = (*bal)()) return (*alive)(); else return yasync::completed(virt::ProxyConnectionFactoryResult::Err(virt::CONFAERREVERY1ISDED)); }, u.second.retries.value_or(0));
						break;
					}
					case LoadBalancingMethod::FailRobin:{
						auto bal = std::make_shared<virt::FailRobinBalancer<virt::ProxyConnectionFactory>>(&lbhmon, true);
						for(auto h : u.second.hosts){
							auto hpfr = fiz::findConnectionFactory(h.address, &ioengine, &ticktack, estconntimeout);
							if(auto err = hpfr.err()){
								std::cerr << "Ups host address lookup error: " << err << "\n";
								okay = false;
								break;
							}
							if(auto err = bal->add(std::move(*hpfr.ok()), h.address, *h.healthCheckUrl, h.weight.value_or(1)).errOpt()){
								std::cerr << "Ups host health check init error: " << *err << "\n";
								okay = false;
								break;
							}
						}
						uv = virt::proxyTo(&engine, [bal](){ if(auto alive = (*bal)()) return (*alive)(); else return yasync::completed(virt::ProxyConnectionFactoryResult::Err(virt::CONFAERREVERY1ISDED)); }, u.second.retries.value_or(0));
						break;
					}
					default:
						std::cerr << "Load balancing method not supported\n";
						okay = false;
						break;
				}
				ups[u.first] = uv;
			}
			std::unordered_map<IPp, virt::R1otBuilder> terms;
			std::unordered_map<IPp, HostMapper<yasync::io::ssl::SharedSSLContext>> sslctx;
			for(auto vhost : config.vhosts){
				if(!okay) break;
				auto stackr = std::visit(overloaded {
					[&](const config::FileServer& fs) -> result<virt::SServer, std::string> { return virt::SServer(new virt::StaticFileServer(&ioengine, fs.root, fs.defaultFile.value_or("index.html"), fs.autoindex.value_or(false))); },
					[&](const config::Proxy& prox){ return std::visit(overloaded {
						[&](const IPp& ipp){ return fiz::findConnectionFactory(ipp, &ioengine, &ticktack, estconntimeout).mapOk([&](auto connf){ return virt::proxyTo(&engine, std::move(connf), 0); }); },
						[&](const std::string& u) -> result<virt::SServer, std::string> { return ups[u]; }
					}, prox.upstream).mapOk([&](auto stack){ return virt::modifyHeaders2(HeadMod{}.rem(http::headerGetStr(http::Header::Host)), HeadMod{}, virt::modifyHeaders2(prox.fwdMod, prox.bwdMod, stack)); }); }
				}, vhost.mode);
				if(auto err = stackr.err()){
					std::cerr << "Error initializing stack: " << *err << "\n";
					okay = false;
					break;
				}
				auto stack = std::move(*stackr.ok());
				if(auto auth = vhost.auth) stack = virt::putBehindBasicAuth(auth->realm, auth->credentials, std::move(stack), vhost.mode.index() == 1);
				auto fiz = &terms[vhost.address];
				fiz->addService(vhost.tok(), stack);
				if(vhost.isDefault) fiz->setDefaultService(stack);
				if(auto ssl = vhost.tls) yasync::io::ssl::createSSLContext(ssl->cert, ssl->key) >> ([&](auto sctx){ sslctx[vhost.address].addHost(sctx, vhost.tok()); }|[&](auto err){ std::cerr << "Failed to initalize VHost ssl context: " << err << "\n"; okay = false; });
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
				std::cout << "All operations online!\n";
				interproc::extNotifyReady(argc, args);
			} else for(auto li : *lists) li->shutdown();
			yasync::blawait<void>(&engine, dun);
			std::cout << "Closing idle connections\n";
			conca.shutdown();
			lbhmon.shutdown();
			ticktack.shutdown();
			ioengine.wioe();
		}
		yasync::io::CtrlC::un(&engine);
		engine.wle();
		std::cout << "Goodbye/\n";
	}
	return 0;
}

}

int main(int argc, char* args[]){
	return redips::main(argc, args);
}
