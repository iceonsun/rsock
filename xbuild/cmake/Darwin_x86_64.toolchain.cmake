set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(DARWIN_TOOLCHAIN_ROOT /usr/local/Cellar/gcc/7.3.0)
set(DARWIN_BINUTILS_ROOT /usr/local/Cellar/binutils/2.30)

set(CMAKE_C_COMPILER ${DARWIN_TOOLCHAIN_ROOT}/bin/gcc-7)
set(CMAKE_CXX_COMPILER ${DARWIN_TOOLCHAIN_ROOT}/bin/g++-7)

# project. static linking c++ library
include(${CMAKE_CURRENT_LIgST_DIR}/base.cmake)