#include "preqh.hpp"

#include <iostream>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <regex>
#include <util/strim.hpp>

namespace redips::fiz {

using namespace magikop;
using namespace http;

ConnectionCare::ConnectionCare(yasync::TickTack* t, std::optional<yasync::TickTack::Duration> idle, std::optional<yasync::TickTack::Duration> trans, std::optional<double> b) : ticktack(t), toIdle(idle), toTrans(trans), minByterate(b){}

inline bool keepAlive(RR& rr){
	if(auto conn = rr.getHeader(Header::Connection)){
		if(*conn == "close") return false;
		std::regex clore("(^|\\W)close($|\\W)");
		std::cmatch m;
		if(std::regex_search(conn->c_str(), m, clore)) return false;
	}
	return true;
}

void ConnectionCare::setIdle(yasync::io::IOResource conn){
	std::unique_lock lok(idlok);
	idle.insert(conn);
}
void ConnectionCare::unsetIdle(yasync::io::IOResource conn){
	std::unique_lock lok(idlok);
	idle.erase(conn);
}

void ConnectionCare::shutdown(){
	sdown = true;
	std::unordered_set<yasync::io::IOResource> ridle;
	{
		std::unique_lock lok(idlok);
		std::swap(ridle, idle);
	}
	for(auto conn : ridle) conn->cancel();
}

static std::string timestamp(){
	auto t = std::time(nullptr);
	auto tm = *std::gmtime(&t); //FIXME not thread-safe
	std::ostringstream tf;
	tf << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S %Z");
	return tf.str();
}

void ConnectionCare::takeCare(ConnectionInfo inf, virt::SServer vs){
	auto conn = inf.connection;
	setIdle(conn);
	auto idleT = toIdle ? ticktack->sleep(*toIdle, [=](auto, bool cancel){ if(!cancel) conn->cancel(); }) : ticktack->UnId;
	conn->engine <<= conn->peek<std::vector<char>>(1) >> [=](auto pr){
		unsetIdle(conn);
		ticktack->stop(idleT);
		return pr;
	} >> ([=](auto rd){
		if(rd.size() == 0){
			std::cerr << "Received hung up conn.\n";
			return;
		}
		const auto startTms = std::chrono::steady_clock::now();
		auto transT = toTrans ? ticktack->sleep(*toTrans, [=](auto, bool cancel){ if(!cancel) conn->cancel(); }) : ticktack->UnId;
		conn->engine <<= RRaw::read(conn) >> [=](auto rr){
			ticktack->stop(transT);
			return rr;
		} >> ([=](SharedRRaw reqwest){
			if(minByterate) if(reqwest->diagRTotalBytes/double(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTms).count()) < *minByterate){
				Response resp(http::Status::REQUEST_TIMEOUT);
				resp.setHeader(Header::Date, timestamp());
				resp.setHeader(Header::Connection, "closed");
				resp.setHeader(Header::TimeoutReason, "Throughput");
				auto wr = conn->writer();
				resp.write(wr);
				conn->engine <<= wr->eod() >> ([=](){}|[](auto err){
					std::cerr << "Failed to inform client of transmission low throughput: " << err << "\n";
				});
				return;
			}
			const bool rkal = keepAlive(*reqwest);
			vs->take(ConnectionInfo{inf.connection, inf.address, inf.protocol, reqwest->getHeader(Header::Host)}, reqwest, [=](auto resp){
				auto wr = conn->writer();
				resp.setHeader(Header::Date, timestamp());
				bool kal = rkal;
				{
					Response respasresp;
					if(respasresp.readTitle(resp.title).isOk()) kal &= respasresp.status != Status::BAD_REQUEST && respasresp.status < Status::INTERNAL_SERVER_ERROR;
				}
				resp.setHeader(Header::Connection, kal && !sdown ? "keep-alive" : "closed");
				resp.write(wr);
				conn->engine <<= wr->eod() >> ([=](){
					if(kal && !sdown) takeCare(inf, vs); //If shut down is initiated after keep-alive is sent, oh well. May as well close it now.
				}|[](auto err){
					std::cerr << "Failed to respond: " << err << "\n";
				});
			});
		}|[=](auto err){
			if(err.asstat){
				Response resp(*err.asstat);
				resp.setHeader(Header::Date, timestamp());
				resp.setHeader(Header::Connection, "closed");
				auto wr = conn->writer();
				resp.write(wr);
				conn->engine <<= wr->eod() >> ([=](){}|[](auto err){
					std::cerr << "Failed to inform client of malformed request: " << err << "\n";
				});
			} else if(beginsWith(err.desc, "Operation cancelled")){
				Response resp(http::Status::REQUEST_TIMEOUT);
				resp.setHeader(Header::Date, timestamp());
				resp.setHeader(Header::Connection, "closed");
				resp.setHeader(Header::TimeoutReason, "Transaction");
				auto wr = conn->writer();
				resp.write(wr);
				conn->engine <<= wr->eod() >> ([=](){}|[](auto err){
					std::cerr << "Failed to inform client of transmission timeout: " << err << "\n";
				});
			} else std::cerr << "Read request error " << err.desc << "\n";
		});
	}|[=](auto err){
		if(beginsWith(err, "Operation cancelled")){
			Response resp(http::Status::REQUEST_TIMEOUT);
			resp.setHeader(Header::Date, timestamp());
			resp.setHeader(Header::Connection, "closed");
			resp.setHeader(Header::TimeoutReason, "Keep-Alive");
			auto wr = conn->writer();
			resp.write(wr);
			conn->engine <<= wr->eod() >> ([=](){}|[](auto err){
				std::cerr << "Failed to inform client of idle timeout: " << err << "\n";
			});
		} else std::cerr << "Idle conn peek error " << err << "\n";
	});
}

}
