#include "timout.hpp"

namespace redips::virt {

using namespace magikop;
using namespace redips::http;

class TimIsOut : public IServer {
	yasync::Yengine* engine;
	yasync::TickTack* ticktack;
	yasync::TickTack::Duration to;
	SServer deleg;
	public:
		TimIsOut(yasync::Yengine* e, yasync::TickTack* t, const yasync::TickTack::Duration& d, SServer s) : engine(e), ticktack(t), to(d), deleg(s) {}
		void take(const ConnectionInfo& conn, SharedRRaw req, RespBack respb) override {
			auto hwar = std::make_shared<yasync::OutsideFuture<RRaw>>();
			engine <<= hwar >> [respb](auto resp){ respb(std::move(resp)); };
			auto t = ticktack->sleep(to, [=](auto, bool cancel){
				if(!cancel && hwar->state() != yasync::FutureState::Completed){
					hwar->set(yasync::FutureState::Completed, Response(Status::GATEWAY_TIMEOUT));
					engine->notify(hwar);
				}
			});
			deleg->take(conn, req, [=](auto resp){
				if(hwar->state() != yasync::FutureState::Completed){
					ticktack->stop(t);
					hwar->set(yasync::FutureState::Completed, std::move(resp));
				}
			});
		}
};

SServer timeItOut(yasync::Yengine* engine, yasync::TickTack* ticktack, const yasync::TickTack::Duration& d, SServer s){
	return SServer(new TimIsOut(engine, ticktack, d, s));
}

}
