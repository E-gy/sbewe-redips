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
				conn->engine->engine <<= http::Request::read(conn) >> [=](auto rr){
					if(auto ok = rr.ok()){
						auto resp = http::Response();
						resp.version = http::Version::HTTP11;
						resp.status = http::Status::OK;
						resp.setHeader("Content-Type", "text");
						std::string respb = "hello world!";
						resp.body = std::vector<char>(respb.begin(), respb.end());
						auto wr = conn->writer();
						resp.write(wr);
						conn->engine->engine <<= wr->eod() >> [](auto wrr){
							if(wrr.isOk()) std::cout << "Sent response!\n";
							else std::cerr << "Send response error " << wrr.err() << "\n"; 
						};
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
