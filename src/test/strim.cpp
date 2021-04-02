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

SCENARIO("Remove Dot Segments", "[uri][util]"){
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

SCENARIO("Decode percent characters", "[uri][util]"){
	GIVEN("encoded path"){
		auto c = GENERATE(
			Case { "hello.", "hello." },
			Case { "h%65llo.", "hello." },
			Case { "h%65llo%2E", "hello." },
			Case { "h%65llo%2e", "hello." },
			Case { "%21h%65llo%2e", "%21hello." },
			Case { "%21h%65llo%2e%20%2F", "%21hello.%20%2F" },
			Case { "%E5%86%86%C2%A3", "円£" },
			Case { "", "" }
		);
		WHEN("decoding unreserved characters"){
			auto seg = c.in;
			decodeUnreservedPercent(seg);
			REQUIRE(seg == c.expected);
		}
	}
}

}
