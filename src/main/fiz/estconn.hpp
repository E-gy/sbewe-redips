#pragma once

#include <yasync.hpp>
#include <yasync/ticktack.hpp>
#include <util/ip.hpp>
#include <functional>

namespace redips::fiz::estconn {

using namespace magikop;

using EstConner = std::function<yasync::Future<yasync::io::ConnectionResult>()>;

/**
 * @returns () → Future<ConnectionResult>
 */
template<int SDomain, int SType, int SProto, typename AddressInfo> auto estConner(yasync::io::NetworkedAddressInfo && addr, yasync::io::IOYengine* engine, yasync::TickTack* ticktack, const yasync::TickTack::Duration& timeout){
	return [addr = std::make_shared<yasync::io::NetworkedAddressInfo>(std::move(addr)), engine, ticktack, timeout](){
		return yasync::io::netConnectTo<SDomain, SType, SProto, AddressInfo>(engine, addr.get()) >> ([=](auto econns){
			auto tEC = ticktack->sleep(timeout, [=](auto, bool cancel){ if(!cancel) econns->cancel(); });
			return econns->connest() >> [=](auto connr){
				ticktack->stop(tEC);
				return connr;
			};
		}|[](auto err){
			return yasync::completed(yasync::io::ConnectionResult::Err(err));
		});
	};
}

result<EstConner, std::string> findEstConner(const IPp& ipp, yasync::io::IOYengine* engine, yasync::TickTack* ticktack, const yasync::TickTack::Duration& timeout);

}
