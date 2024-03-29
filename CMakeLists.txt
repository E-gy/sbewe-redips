cmake_minimum_required(VERSION 3.11)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.cmake")
Include(FetchContent)

project(sbewe-redips-core VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 17)
add_compile_definitions("$<$<CONFIG:DEBUG>:_DEBUG>")

find_program( LCOV_PATH  NAMES lcov lcov.bat lcov.exe lcov.perl)
if(LCOV_PATH)
	Include(CodeCoverage)
endif()

find_package(Threads)
find_package(OpenSSL)

SET (CMAKE_ENABLE_EXPORTS TRUE)

# YAsynC

file(GLOB_RECURSE src_yasync "${PROJECT_SOURCE_DIR}/src/yasync/*.hpp" "${PROJECT_SOURCE_DIR}/src/yasync/*.cpp")
add_library(yasync OBJECT ${src_yasync})
include_directories(yasync ${PROJECT_SOURCE_DIR}/src/yasync)
target_compile_options(yasync PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror -pedantic>
)

# Core

file(GLOB_RECURSE src_main "${PROJECT_SOURCE_DIR}/src/main/*.hpp" "${PROJECT_SOURCE_DIR}/src/main/*.cpp")
add_library(sbewe-redips-core STATIC ${src_main} $<TARGET_OBJECTS:yasync>)
include_directories(sbewe-redips-core ${PROJECT_SOURCE_DIR}/lib)
include_directories(sbewe-redips-core ${PROJECT_SOURCE_DIR}/src/yasync)
include_directories(sbewe-redips-core ${PROJECT_SOURCE_DIR}/src/main)
target_compile_options(sbewe-redips-core PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror -pedantic>
)
target_link_libraries(sbewe-redips-core ${CMAKE_THREAD_LIBS_INIT} OpenSSL::SSL)

add_library(sbewe-redips-core-cov STATIC ${src_main} $<TARGET_OBJECTS:yasync>)
include_directories(sbewe-redips-core-cov ${PROJECT_SOURCE_DIR}/lib)
include_directories(sbewe-redips-core-cov ${PROJECT_SOURCE_DIR}/src/yasync)
include_directories(sbewe-redips-core-cov ${PROJECT_SOURCE_DIR}/src/main)
target_compile_options(sbewe-redips-core-cov PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror -pedantic>
)
target_compile_options(sbewe-redips-core-cov PUBLIC --coverage)
target_link_libraries(sbewe-redips-core-cov ${CMAKE_THREAD_LIBS_INIT} OpenSSL::SSL gcov)

if(WIN32)
	target_link_libraries(sbewe-redips-core ws2_32 "$<$<CONFIG:DEBUG>:dbghelp>")
	target_link_libraries(sbewe-redips-core-cov ws2_32 "$<$<CONFIG:DEBUG>:dbghelp>")
endif()

# Exe

file(GLOB_RECURSE src_exe "${PROJECT_SOURCE_DIR}/src/exe/*.hpp" "${PROJECT_SOURCE_DIR}/src/exe/*.cpp")

add_executable(sbewe-redips ${src_exe})
include_directories(sbewe-redips ${PROJECT_SOURCE_DIR}/lib)
include_directories(sbewe-redips ${PROJECT_SOURCE_DIR}/src/main)
include_directories(sbewe-redips ${PROJECT_SOURCE_DIR}/src/exe)
target_compile_options(sbewe-redips PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror -pedantic>
)
target_link_libraries(sbewe-redips sbewe-redips-core)
set_property(TARGET sbewe-redips PROPERTY POSITION_INDEPENDENT_CODE TRUE)

add_executable(sbewe-redips-cov ${src_exe})
include_directories(sbewe-redips-cov ${PROJECT_SOURCE_DIR}/lib)
include_directories(sbewe-redips-cov ${PROJECT_SOURCE_DIR}/src/main)
include_directories(sbewe-redips-cov ${PROJECT_SOURCE_DIR}/src/exe)
target_compile_options(sbewe-redips-cov PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror -pedantic>
)
target_link_libraries(sbewe-redips-cov sbewe-redips-core-cov)

# Tests

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/crt)
add_custom_command(
	OUTPUT ./crt/server.crt ./crt/server.key ./crt/client.crt ./crt/client.key
	COMMAND "${PROJECT_SOURCE_DIR}/certs/gen.$<IF:$<BOOL:${WIN32}>,bat,sh>"
	WORKING_DIRECTORY "./crt"
)
add_custom_target(gen-test-crts DEPENDS ./crt/server.crt ./crt/server.key ./crt/client.crt ./crt/client.key)

file(GLOB_RECURSE src_test "${PROJECT_SOURCE_DIR}/src/test/*.hpp" "${PROJECT_SOURCE_DIR}/src/test/*.cpp")
add_executable(sbewe-redips-tests ${src_test})
target_compile_options(sbewe-redips-tests PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wno-unused -Wno-missing-field-initializers -Werror>
)
add_dependencies(sbewe-redips-tests gen-test-crts)

# Catch2v3 from Fetch works properly, nice 
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.0-preview3)
FetchContent_MakeAvailable(Catch2)
FetchContent_GetProperties(Catch2 SOURCE_DIR src_Catch2)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${src_Catch2}/extras/")
target_link_libraries(sbewe-redips-tests Catch2::Catch2WithMain)

include_directories(sbewe-redips-tests ${PROJECT_SOURCE_DIR}/lib)
include_directories(sbewe-redips-tests ${PROJECT_SOURCE_DIR}/src/main)
include_directories(sbewe-redips-tests ${PROJECT_SOURCE_DIR}/src/test)
target_link_libraries(sbewe-redips-tests sbewe-redips-core-cov)
if(LCOV_PATH)
	setup_target_for_coverage_lcov(
		NAME sbewe-redips-tests-cov
		EXECUTABLE sbewe-redips-tests
		BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/src"
		EXCLUDE "lib/*"
	)
endif()

set(CTEST_OUTPUT_ON_FAILURE 1)
include(CTest)
include(Catch)
catch_discover_tests(sbewe-redips-tests)
