set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER gcc-6)
set(CMAKE_CXX_COMPILER g++-6)

# project. static linking c++ library
include(${CMAKE_CURRENT_LIST_DIR}/base.cmake)
