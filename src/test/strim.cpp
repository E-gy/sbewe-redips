#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

#if __INTELLISENSE__
#pragma diag_suppress 2486
#endif

#include <util/strim.hpp>

namespace test {

struct Case {
	std::string in;
	std::string expected;
};

SCENARIO("Remove Dot Segments", "[http][util]"){
	GIVEN("unresolved relative path"){
		auto c = GENERATE(
			Case { "/a/b/c/./../../g", "/a/g" },
			Case { "mid/content=5/../6", "mid/6" },
			Case { "/hello/world", "/hello/world" },
			Case { "../hello/world", "/hello/world" },
			Case { "./hello/world", "/hello/world" },
			Case { "./../hello/world", "/hello/world" },
			Case { "./../hello/world/.", "/hello/world" },
			Case { "./../hello/world/../..", "/" },
			Case { "/.", "/" },
			Case { "/..", "/" },
			Case { "/", "/" }
		);
		WHEN("removing dot segments"){
			auto seg = c.in;
			removeDotSegments(seg);
			REQUIRE(seg == c.expected);
		}
	}
}

}
