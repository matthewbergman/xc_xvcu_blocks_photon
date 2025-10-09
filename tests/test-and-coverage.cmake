if(WIN32)
	set(GRCOV_PKG grcov-x86_64-pc-windows-msvc.zip)
	set(GRCOV_HASH 33c28588407cc89d1923cc1771ac9bcc625a2b111dbc45395be163a1988e7fcf)
	set(POSTFIX ".exe")
	set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/clang-tools.cmake)
	set(LLVM_COV "llvm-cov")
	set(FIND_TOOL "C:/Program Files/Git/usr/bin/find.exe")
	add_compile_definitions(
	    -DGTEST_LINKED_AS_SHARED_LIBRARY=0
	    -DGTEST_CREATE_SHARED_LIBRARY=0
	)
	option(CMAKE_USE_WIN32_THREADS_INIT "Using Win32 threads" ON)
	set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
else()
	set(GRCOV_PKG grcov-x86_64-unknown-linux-gnu.tar.bz2)
	set(GRCOV_HASH 098be4d60b8016913542d58456fea6e771890096d1bf86e7f83dac650ba4b58a)
	set(POSTFIX "")
	set(LLVM_COV "")
	set(FIND_TOOL "find")
endif()

include(FetchContent)

FetchContent_Declare(
	grcov
	URL https://github.com/mozilla/grcov/releases/download/v0.8.19/${GRCOV_PKG}
	URL_HASH SHA256=${GRCOV_HASH} 
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(
	googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(COVERAGE_TOOL ${CMAKE_CURRENT_BINARY_DIR}/_deps/grcov-src/grcov${POSTFIX})
FetchContent_MakeAvailable(googletest grcov)
