#include "interproc.hpp"

#include <cstring>

namespace redips::interproc {

void extNotifyReady([[maybe_unused]] int argc, [[maybe_unused]] char* args[]){
	std::strcpy(args[0], "[READY]");
}

}
