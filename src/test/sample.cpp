#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

#include <sample.hpp>

#if __INTELLISENSE__
#pragma diag_suppress 2486
#endif

using namespace redips;

SCENARIO("integer mul 2", "[mul2][sample]"){
	GIVEN("an integer"){
		int i = GENERATE(1, 2, 3, 0, -100, 1554);
		CAPTURE(i);
		THEN("mul2 is multiplication by 2"){
			REQUIRE(mul2(i) == i*2);
		}
	}
}
