cmake_minimum_required(VERSION 3.22)
include(test-and-coverage.cmake)
project(application_tests LANGUAGES C CXX)
set(TEST_TARGET xvcu_library_tests)
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/..)

# Build unit under test as static library

include(../cmake/xvcu_library.cmake)

target_compile_options(xvcu_blocks_photon PUBLIC 
	-fprofile-arcs
	-ftest-coverage
)

target_link_options(xvcu_blocks_photon PUBLIC 
	--coverage
)

# Build unit-test executable
add_executable(${TEST_TARGET}
	${SOURCE_DIR}/blocks/kreisel_bms/tests/test_kreisel_bms.cpp
)

# Link test executable to tested library
target_link_libraries(${TEST_TARGET} PRIVATE
	xvcu_blocks_photon
	GTest::gtest_main
	GTest::gmock
)

# Automatically execute unit tests upon successful build.

enable_testing()
include(GoogleTest)
gtest_discover_tests(${TEST_TARGET})
add_custom_command(TARGET ${TEST_TARGET} POST_BUILD
	COMMAND ctest ARGS --output-on-failure
)
