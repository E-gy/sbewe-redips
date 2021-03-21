#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <yasync/iompls.hpp>
#include <yasync/impls.hpp>
#include <ossl/iores.hpp>

#if __INTELLISENSE__
#pragma diag_suppress 2486
#endif

namespace test {

using namespace yasync;
using namespace yasync::io;
using namespace yasync::io::ssl;
using namespace magikop;

constexpr auto PAT = "\r\n\r\n";
constexpr auto PATLEN = 4;
constexpr auto GENEROK = "OK";
constexpr auto ERRPREF = "An Error Occured: ";

SCENARIO("SSL Hanshake Simulator", "[ssl][io][yasync]"){
	SSLStateControl _ssl;
	{
		Yengine engine(GENERATE(1)); //FIXME! more threads simply doesn't work :/
		{
			auto sctx = createSSLContext("./crt/server.crt", "./crt/server.key") >> ([](auto ctx){ return ctx; }|[](auto err) -> SharedSSLContext { throw std::runtime_error(err); });
			auto cctx = createSSLContext("./crt/client.crt", "./crt/client.key") >> ([](auto ctx){ return ctx; }|[](auto err) -> SharedSSLContext { throw std::runtime_error(err); });
			GIVEN("Server and Client SSL configuration"){
				auto ioi = ioi2Way(&engine);
				AND_GIVEN("an IOI"){
					auto sssl = SSL_new(sctx->ctx());
					auto cssl = SSL_new(cctx->ctx());
					SSL_set_accept_state(sssl);
					SSL_set_connect_state(cssl);
					auto sio = openSSLIO(std::move(ioi.first), sssl) >> ([](auto io){ return io; }|[](auto err) -> IOResource { throw std::runtime_error(err); });
					auto cio = openSSLIO(std::move(ioi.second), cssl) >> ([](auto io){ return io; }|[](auto err) -> IOResource { throw std::runtime_error(err); });
					SSL_accept(sssl);
					SSL_connect(cssl);
					auto direction = GENERATE(std::pair<unsigned, unsigned>(0, 1), std::pair<unsigned, unsigned>(1, 0));
					CAPTURE(direction);
					auto io1 = direction.first == 0 ? std::move(sio) : std::move(cio);
					auto io2 = direction.second == 0 ? std::move(sio) : std::move(cio);
					WHEN("Sending data in one direction (patern delimited)"){
						std::string message(GENERATE("hi", "Hello secure world!", "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam pretium aliquam facilisis. Integer semper maximus ligula, quis congue sem viverra ut. Cras sollicitudin rhoncus lectus, efficitur venenatis nunc fringilla sit amet. Donec at pretium dolor. Phasellus eleifend elit auctor nulla pulvinar laoreet. Praesent commodo, dolor sit amet venenatis gravida, velit lectus efficitur mi, quis maximus mi sem eu magna. Pellentesque nunc augue, interdum sit amet dui sit amet, commodo ultricies felis. Morbi a ante quis est fringilla congue non in tortor. Aliquam malesuada ante eu massa mollis vulputate. In hac habitasse platea dictumst. Vestibulum consequat est ligula, eget mattis nunc eleifend sit amet. Nunc dictum nisl non sem egestas eleifend. Phasellus convallis ligula vel ante interdum euismod. Proin non erat porttitor, ullamcorper mauris in, dignissim libero. Integer tincidunt dignissim feugiat."));
						auto awaw = engine <<= io1->write(message+PAT) >> ([](){ return GENEROK; }|[](auto err){ return ERRPREF + err; });
						auto awar = engine <<= io2->read<std::string>(PAT) >> ([](auto str){ return str; }|[](auto err){ return ERRPREF + err; });
						THEN("what is sent is received"){
							REQUIRE_THAT(blawait(&engine, awaw), Catch::Matchers::Equals(GENEROK));
							REQUIRE_THAT(blawait(&engine, awar), Catch::Matchers::Equals(message+PAT));
						}
					}
					/*WHEN("Sending data in one direction (length delimited)"){ FIXME!!!
						auto message = GENERATE(std::vector<char>{1, 15, 29, 26, 69});
						auto awaw = engine <<= io1->write<std::vector<char>>(message) >> ([](){ return GENEROK; }|[](auto err){ return ERRPREF + err; });
						auto awar = engine <<= io2->read<std::vector<char>>(message.size()) >> ([](auto vec){ return vec; }|[](auto err){ return std::vector<char>{}; });
						THEN("what is sent is received"){
							REQUIRE_THAT(blawait(&engine, awaw), Catch::Matchers::Equals(GENEROK));
							REQUIRE(blawait(&engine, awar) == message);
						}
					}*/
					WHEN("Sending data both ways (patern delimited)"){
						std::string message(GENERATE("hi", "Hello secure world!", "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam pretium aliquam facilisis. Integer semper maximus ligula, quis congue sem viverra ut. Cras sollicitudin rhoncus lectus, efficitur venenatis nunc fringilla sit amet. Donec at pretium dolor. Phasellus eleifend elit auctor nulla pulvinar laoreet. Praesent commodo, dolor sit amet venenatis gravida, velit lectus efficitur mi, quis maximus mi sem eu magna. Pellentesque nunc augue, interdum sit amet dui sit amet, commodo ultricies felis. Morbi a ante quis est fringilla congue non in tortor. Aliquam malesuada ante eu massa mollis vulputate. In hac habitasse platea dictumst. Vestibulum consequat est ligula, eget mattis nunc eleifend sit amet. Nunc dictum nisl non sem egestas eleifend. Phasellus convallis ligula vel ante interdum euismod. Proin non erat porttitor, ullamcorper mauris in, dignissim libero. Integer tincidunt dignissim feugiat."));
						auto awao = engine <<= io1->write<std::string>(message+PAT) >> ([engine = &engine, io1](){
							return engine <<= io1->read<std::string>(PAT) >> ([](auto resp){ return resp; }|[](auto err){ return ERRPREF+err; });
						}|[](auto err){ return completed(ERRPREF+err); });
						auto awam = engine <<= io2->read<std::string>(PAT) >> ([engine = &engine, io2](auto str){
							auto substr = str.substr(0, (str.length() - PATLEN)/2);
							return engine <<= io2->write(substr+substr+PAT) >> ([](){ return GENEROK; }|[](auto err){ return ERRPREF+err; });
						}|[](auto err){ return completed(ERRPREF+err); });
						THEN("what is sent is received"){
							auto submess = message.substr(0, message.length()/2);
							REQUIRE_THAT(blawait(&engine, awam), Catch::Matchers::Equals(GENEROK));
							REQUIRE_THAT(blawait(&engine, awao), Catch::Matchers::Equals(submess+submess+PAT));
						}
					}
				}
			}
		}
		engine.wle();
	}
}

}
