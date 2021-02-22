#include <sample.hpp>

#include <util/interproc.hpp>

using namespace redips;

int main(int argc, char* args[]){
	interproc::extNotifyReady(argc, args);
	return mul2(0);
}
