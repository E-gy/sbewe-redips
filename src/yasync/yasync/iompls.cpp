#include "iompls.hpp"

#include "util.hpp"
#include <mutex>

namespace yasync::io {

class IOI2Way {
	std::vector<char> buff[2];
	std::mutex locks[2];
	bool exi[2] = {false, false};
	std::optional<std::shared_ptr<OutsideFuture<void>>> notifi[2];
	IOYengine* notivia[2] = {nullptr, nullptr};
	template<unsigned W> void notify(){
		if(notifi[W]){
			auto n = *notifi[W];
			n->s = FutureState::Completed;
			notivia[W]->engine->notify<void>(n);
		}
	}
	template<unsigned R, unsigned W> class IOI : public IAIOResource {
		friend class IOI2Way;
		std::shared_ptr<IOI2Way> share;
		std::weak_ptr<IOI> slf;
		std::weak_ptr<IOI<W, R>> oe;
		IOI(IOYengine* e, std::shared_ptr<IOI2Way> p) : IAIOResource(e), share(p) {
			share->notivia[R] = e;
			share->exi[W] = true;
		}
		public:
			~IOI(){
				{	//Clean up self write channel. aka Notify the other end of EOT
					std::unique_lock lk(share->locks[W]);
					share->exi[W] = false;
					share->notify<W>();
				}
				{	//Clean up self read channel. In solid theory, not needed
					std::unique_lock lk(share->locks[R]);
					share->notifi[R] = std::nullopt;
					share->notivia[R] = nullptr;
				}
			}
			void notify([[maybe_unused]] IOCompletionInfo _) override {}
			Future<ReadResult> _read(size_t bytes = 0) override {
				return defer(lambdagen([this, self = slf.lock(), bytes]([[maybe_unused]] const Yengine* _eng, bool& done, std::vector<char> data) -> std::variant<AFuture, movonly<ReadResult>> {
					if(done) return ReadResult::Ok(data);
					share->notifi[R] = std::nullopt;
					std::unique_lock lk(share->locks[R]);
					MoveAppend(share->buff[R], data);
					if((bytes > 0 && data.size() >= bytes) || !share->exi[R]){
						done = true;
						return ReadResult::Ok(data);
					}
					return *(share->notifi[R] = std::make_shared<OutsideFuture<void>>());
				}, std::vector<char>()));
			}
			Future<WriteResult> _write(std::vector<char>&& data) override {
				return completed<std::vector<char>>(std::move(data)) >> [this, self = slf.lock()](auto data){
					std::unique_lock lk(share->locks[W]);
					MoveAppend(data, share->buff[W]);
					share->notify<W>();
					return WriteResult::Ok();
				};
			}
	};
	public:
		static std::pair<IOResource, IOResource> newt(IOYengine* e1, IOYengine* e2){
			auto p = std::shared_ptr<IOI2Way>(new IOI2Way());
			return std::pair<IOResource, IOResource>{
				std::shared_ptr<IOI<0, 1>>(new IOI<0, 1>(e1, p)),
				std::shared_ptr<IOI<1, 0>>(new IOI<1, 0>(e2, p)),
			};
		}
};

std::pair<IOResource, IOResource> ioi2Way(IOYengine* e1, IOYengine* e2){
	return IOI2Way::newt(e1, e2);
}

}

