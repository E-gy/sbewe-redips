#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

#include <yasync/iompls.hpp>
#include <yasync/impls.hpp>

#if __INTELLISENSE__
#pragma diag_suppress 2486
#endif

using namespace yasync;
using namespace yasync::io;

SCENARIO("ioi until EOF", "[io][yasync]"){
	Yengine engine(2);
	{
		auto ioi = ioi2Way(&engine);
		GIVEN("an IOI"){
			WHEN("reading until EOF"){
				auto red = ioi.second->read<std::string>();
				WHEN("writing the closing the other end"){
					{
						auto wr = std::move(ioi.first);
						&engine <<= wr->write<std::string>("Lorem ipsum hey wait not again!");
					}
					THEN("the read eventually completes with the data"){
						auto dat = blawait(&engine, red);
						REQUIRE(dat.isOk());
						REQUIRE(*dat.ok() == "Lorem ipsum hey wait not again!");
					}
				}
			}
		}
	}
}
