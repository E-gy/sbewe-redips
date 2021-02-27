#include <sample.hpp>

#include <yasync.hpp>
#include <http/rr.hpp>
#include <util/interproc.hpp>
#include <config/config.hpp>

#include <iostream>
#include <fstream>

using namespace redips;
using namespace yasync;
using namespace yasync::io;
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
	SystemNetworkingStateControl _sysnet;
	{
		Yengine engine(4);
		auto timeToStahpRes = CtrlC::on(&engine);
		if(auto timeToStahp = timeToStahpRes.ok()){
			IOYengine ioengine(&engine);
			netListen<AF_INET, SOCK_STREAM, IPPROTO_TCP, sockaddr_in>(&ioengine, []([[maybe_unused]] auto lsock, sysneterr_t err, const std::string& location){
				std::cerr << "Listen error: " << printSysNetError(location, err) << "\n";
				return false;
			}, []([[maybe_unused]] const sockaddr_in& remote, const IOResource& conn){
				std::cout << "conn!\n";
				conn->engine->engine <<= http::Request::read(conn) >> ([=](http::SharedRequest reqwest){
					std::cout << "Received request!\n";
					//Proxy go!
					NetworkedAddressInfo::find<AF_INET, SOCK_STREAM, IPPROTO_TCP>("93.184.216.34", "80") >> ([=](auto addrspace){
						netConnectTo<AF_INET, SOCK_STREAM, IPPROTO_TCP, sockaddr_in>(conn->engine, &addrspace) >> ([=](auto connf){
							conn->engine->engine <<= connf >> ([=](auto proxyconn){
								std::cout << "Proxied connection ready wooo!!!\n";
								auto w = proxyconn->writer();
								reqwest->write(w);
								conn->engine->engine <<= w->eod() >> ([=](){
									std::cout << "Sent proxied request!\n";
									conn->engine->engine <<= http::Response::read(proxyconn) >> ([=](auto respp){
										std::cout << "Received proxied response!\n";
										auto w = conn->writer();
										respp->write(w);
										conn->engine->engine <<= w->eod() >> ([=](){
											std::cout << "Forwarded response!\n";
										}|[](auto err){ std::cerr << "Forward response error " << err << "\n"; });
									}|[](auto err){ std::cerr << "read proxied response error " << err.desc << "\n"; });
								}|[](auto err){ std::cerr << "send proxied request error " << err << "\n"; });
							}|[](auto err){ std::cerr << "failed to proxied connect (after await): " << err << "\n"; });
						}|[](auto err){ std::cerr << "failed to proxied connect: " << err << "\n"; });
					}|[](auto err){ std::cerr << "failed to get proxied address space: " << err << "\n"; });
				}|[=](auto err){
					std::cerr << "Read error " << err.desc << "\n";
				});
				return result<void, std::string>::Ok();
			}) >> ([&](auto listener){
				NetworkedAddressInfo::find<AF_INET, SOCK_STREAM, IPPROTO_TCP>("localhost", "5050") >> ([&](auto addrspace){
					listener->listen(&addrspace) >> ([&](auto listenp){
						std::cout << "listening...\n";
						interproc::extNotifyReady(argc, args);
						engine <<= *timeToStahp >> [=]{
							std::cout << "initating shut down\n";
							listener->shutdown();
						};
						blawait(&engine, listenp);
						std::cout << "done listening\n";
					}|[](auto err){ std::cerr << "failed to listen: " << err << "\n"; });
				}|[](auto err){ std::cerr << "failed to get address space: " << err << "\n"; });
			}|[](auto err){ std::cerr << "failed to create listening socket: " << err << "\n"; });
			CtrlC::un(&engine);
		} else std::cerr << "failed to set up ctrl+c listener: " << *timeToStahpRes.err() << "\n";
		std::cout << "putting an end to the work loop\n";
		engine.wle();
	}
	std::cout << "slaying d~~a~~emons\n";
	Daemons::waitForEveryoneToStop();
}
