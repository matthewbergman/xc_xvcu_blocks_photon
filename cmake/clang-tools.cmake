# Toolchain File 
# LLVM Clang CrossCompiler tools
# Supports GCC style test coverage reporting 

set(MSG_SET_BUILD_TOOLS_ENV 
	"LLVM Build tool location is defined by a user-set environment variable `LLVM`.\n"
	"This variable represents the path to your local LLVM clang 'LLVM' directory.\n"
	"\n"
	"Example Win32: `LLVM=C:\\Program Files\\LLVM`"
)

if(DEFINED ENV{LLVM})
	set(TOOLING_DIRECTORY $ENV{LLVM})
	set(CXX_COMPILER_NAME clang++)
	set(C_COMPILER_NAME clang)
else()
	message(FATAL_ERROR ${MSG_SET_BUILD_TOOLS_ENV} )
endif()

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_CXX_COMPILER "${TOOLING_DIRECTORY}/bin/${CXX_COMPILER_NAME}.exe")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS --std=c++${CMAKE_CXX_STANDARD})

set(CMAKE_C_COMPILER "${TOOLING_DIRECTORY}/bin/${C_COMPILER_NAME}.exe")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS --std=c${CMAKE_C_STANDARD})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
