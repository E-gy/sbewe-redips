#include <sample.hpp>

#include <yasync.hpp>
#include <http/rr.hpp>
#include <util/interproc.hpp>

#include <iostream>

using namespace redips;
using namespace yasync;
using namespace yasync::io;

int main(int argc, char* args[]){
	SystemNetworkingStateControl _sysnet;
	{
		Yengine engine(4);
		auto timeToStahpRes = CtrlC::on(&engine);
		if(auto timeToStahp = timeToStahpRes.ok()){
			IOYengine ioengine(&engine);
			auto listenerr = netListen<AF_INET, SOCK_STREAM, IPPROTO_TCP, sockaddr_in>(&ioengine, []([[maybe_unused]] auto lsock, sysneterr_t err, const std::string& location){
				std::cerr << "Listen error: " << printSysNetError(location, err) << "\n";
				return false;
			}, []([[maybe_unused]] const sockaddr_in& remote, const IOResource& conn){
				std::cout << "conn!\n";
				conn->engine->engine <<= http::Request::read(conn) >> [=](auto rr){
					if(rr.isOk()){
						std::cout << "Received request!\n";
						http::SharedRequest reqwest = std::move(*rr.ok());
						//Proxy go!
						::addrinfo hints = {};
						hints.ai_family = AF_INET;
						hints.ai_socktype = SOCK_STREAM;
						hints.ai_protocol = IPPROTO_TCP;
						auto addrspaceres = NetworkedAddressInfo::find("93.184.216.34", "80", hints);
						if(auto addrspace = addrspaceres.ok()){
							sockaddr_in bind0 = {};
							bind0.sin_family = AF_INET;
							bind0.sin_addr.s_addr = INADDR_ANY;
							bind0.sin_port = 0;
							auto conres = netConnectTo<AF_INET, SOCK_STREAM, IPPROTO_TCP>(conn->engine, addrspace, bind0);
							if(auto connf = conres.ok()){
								conn->engine->engine <<= *connf >> [=](ConnectionResult connar){
									if(connar.isOk()){
										auto proxyconn = *connar.ok();
										std::cout << "Proxied connection ready wooo!!!\n";
										auto w = proxyconn->writer();
										reqwest->write(w);
										conn->engine->engine <<= w->eod() >> [=](auto wrr){
											if(wrr.isOk()){
												std::cout << "Sent proxied request!\n";
												conn->engine->engine <<= http::Response::read(proxyconn) >> [=](auto prr){
													if(prr.isOk()){
														std::cout << "Received proxied response!\n";
														auto respp = std::move(*prr.ok());
														auto w = conn->writer();
														respp->write(w);
														conn->engine->engine <<= w->eod() >> [=](auto wrr){
															if(wrr.isOk()) std::cout << "Forwarded response!\n";
															else std::cerr << "Forward response error " << *wrr.err() << "\n";
														};
													} else std::cerr << "read proxied response error " << prr.err()->desc << "\n";
												};
											} else std::cerr << "send proxied request error " << *wrr.err() << "\n";
										};
									} else std::cerr << "failed to proxied connect (after await): " << *connar.err() << "\n";
								};
								std::this_thread::sleep_for(std::chrono::milliseconds(10000));
							} else std::cerr << "failed to proxied connect: " << *conres.err() << "\n";
						} else std::cerr << "failed to get proxied address space: " << *addrspaceres.err() << "\n";
					} else std::cerr << "Read error " << rr.err()->desc << "\n";
				};
				return result<void, std::string>::Ok();
			});
			if(auto listenerp = listenerr.ok()){
				auto listener = *listenerp;
				::addrinfo hints = {};
				hints.ai_family = AF_INET;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_protocol = IPPROTO_TCP;
				auto addrspaceres = NetworkedAddressInfo::find("localhost", "5050", hints);
				if(auto addrspace = addrspaceres.ok()){
					auto listener4 = listener->listen(addrspace);
					if(auto listenp = listener4.ok()){
						std::cout << "listening...\n";
						interproc::extNotifyReady(argc, args);
						engine <<= *timeToStahp >> [=]{
							std::cout << "initating shut down\n";
							listener->shutdown();
						};
						blawait(&engine, *listenp);
						std::cout << "done listening\n";
					} else std::cerr << "failed to listen: " << *listener4.err() << "\n";
				} else std::cerr << "failed to get address space: " << *addrspaceres.err() << "\n";
			} else std::cerr << "failed to create listening socket: " << *listenerr.err() << "\n";
			CtrlC::un(&engine);
		} else std::cerr << "failed to set up ctrl+c listener: " << *timeToStahpRes.err() << "\n";
		std::cout << "putting an end to the work loop\n";
		engine.wle();
	}
	std::cout << "slaying d~~a~~emons\n";
	Daemons::waitForEveryoneToStop();
}
