#include "helth.hpp"

#include <http/rr.hpp>
#include <iostream>

namespace redips::fiz {

using namespace std::literals;
using namespace magikop;
using namespace yasync;
using namespace yasync::io;

HealthMonitor::HealthMonitor(IOYengine* e, TickTack::Duration i) : engine(e), interval(i) {}

void HealthMonitor::shutdown(){
	ticktack.shutdown();
}

struct Interim {
	TickTack::Id t = TickTack::UnId;
	TickTack::Id tM = TickTack::UnId;
	TickTack::Id tD = TickTack::UnId;
	ConnectionFactory estconn;
	IOResource conn;
	http::Request req;
	HealthMonitor::SAH sah;
	Interim(ConnectionFactory && e, const std::string& path, HealthMonitor::SAH s) : estconn(std::move(e)), req(http::Method::GET, path), sah(s) {
		req.setHeader(http::Header::Connection, "close");
	}
};

HealthMonitor::SAH HealthMonitor::_add(ConnectionFactory && estconn, const std::string& path){
	const auto sah = std::make_shared<std::atomic<Health>>(Health::Missing);
	const auto interim = std::make_shared<Interim>(std::move(estconn), path, sah);
	auto shr = [=](Health h){
		ticktack.stop(interim->tM);
		ticktack.stop(interim->tD);
		interim->tM = interim->tD = TickTack::UnId;
		sah->store(h);
	};
	auto conok = [=](){
		if(interval/10 > MISSINGT && sah->load() == Health::Alive) interim->tM = ticktack.sleep(MISSINGT, [sah](auto, bool cancel){ if(!cancel) sah->store(Health::Missing); });
		interim->tD = ticktack.sleep(interval/10, [interim](auto, bool cancel){
			if(!cancel){
				if(interim->conn) interim->conn->cancel();
				else interim->sah->store(Health::Dead);
			}
			//FIXME Okay. If the shutdown is initiated after request is emitted and before timeout and if the enpoint stops responding - everything will get blocked until the endpoint closes the connection, or responds
			//The cancel here is not indicative of shutdown vs cancellation on success therefor we can't simply cancel/reset interim.conn, unless we are to re-establish it every ping.
		});
		auto wr = interim->conn->writer();
		interim->req.write(wr);
		engine->engine <<= wr->eod() >> ([=](){
			engine->engine <<= http::Response::read(interim->conn) >> ([=](auto resp){
				if(resp->status == http::Status::OK) shr(Health::Alive);
				else {
					std::cerr << "Health check self reported not OK\n";
					shr(Health::Dead);
				}
				interim->conn.reset();
			}|[=](auto err){
				std::cerr << "Health check read error: " << err.desc << "\n";
				interim->conn.reset();
				shr(Health::Dead);
			});
		}|[=](auto err){
			std::cerr << "Health check send error: " << err << "\n";
			interim->conn.reset();
			shr(Health::Dead);
		});
	};
	interim->t = ticktack.start(TickTack::Clock::now(), interval, [=](auto, bool cancel){
		if(!cancel){
			if(interim->conn) conok();
			else engine->engine <<= interim->estconn() >> ([=](auto conn){
				interim->conn = conn.first;
				interim->req.setHeader(http::Header::Host, conn.second.ip);
				conok();
			}|[=](auto err){
				std::cerr << "Health check connection establish failed: " << err << "\n";
				shr(Health::Dead);
			});
		}
	});
	return sah;
}

result<HealthMonitor::SAH, std::string> HealthMonitor::add(const IPp& ipp, const std::string& path){
	return findConnectionFactory(ipp, engine, &ticktack, interval/10) >> ([=](auto estconn) -> result<HealthMonitor::SAH, std::string> {
		return _add(std::move(estconn), path);
	}|[=](auto err) -> result<HealthMonitor::SAH, std::string> {
		return err;
	});
}

}
