set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(LINUX_TOOLCHAIN_NAME    x86_64-ubuntu16.04-linux-gnu-)
set(LINUX_TOOLCHAIN_ROOT    /Volumes/crosstool-ng/x-tools/x86_64-ubuntu16.04-linux-gnu)
set(CMAKE_SYSROOT           ${LINUX_TOOLCHAIN_ROOT}/x86_64-ubuntu16.04-linux-gnu/sysroot)
set(CMAKE_C_COMPILER        ${LINUX_TOOLCHAIN_ROOT}/bin/${LINUX_TOOLCHAIN_NAME}gcc)
set(CMAKE_CXX_COMPILER      ${LINUX_TOOLCHAIN_ROOT}/bin/${LINUX_TOOLCHAIN_NAME}g++)
set(CMAKE_AR                ${LINUX_TOOLCHAIN_ROOT}/bin/${LINUX_TOOLCHAIN_NAME}ar CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB            ${LINUX_TOOLCHAIN_ROOT}/bin/${LINUX_TOOLCHAIN_NAME}ranlib CACHE FILEPATH "Ranlib")
set(CMAKE_ASM_COMPILER      ${LINUX_TOOLCHAIN_ROOT}/bin/${LINUX_TOOLCHAIN_NAME}as)
set(CMAKE_LINKER            ${LINUX_TOOLCHAIN_ROOT}/bin/${LINUX_TOOLCHAIN_NAME}ld)
set(CMAKE_NM                ${LINUX_TOOLCHAIN_ROOT}/bin/${LINUX_TOOLCHAIN_NAME}nm)
set(CMAKE_OBJCOPY           ${LINUX_TOOLCHAIN_ROOT}/bin/${LINUX_TOOLCHAIN_NAME}objcopy)
set(CMAKE_OBJDUMP           ${LINUX_TOOLCHAIN_ROOT}/bin/${LINUX_TOOLCHAIN_NAME}objdump)
set(CMAKE_STRIP             ${LINUX_TOOLCHAIN_ROOT}/bin/${LINUX_TOOLCHAIN_NAME}strip)

# project. static linking c++ library
set(RSOCK_TOP_BUILD_DIR ${PROJECT_SOURCE_DIR}/xbuild)
set(RSOCK_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${RSOCK_LINKER_FLAGS}")
set(RSOCK_RELEASE TRUE)

include_directories(${RSOCK_TOP_BUILD_DIR}/include/libuv)
include_directories(${RSOCK_TOP_BUILD_DIR}/include/libpcap)
include_directories(${RSOCK_TOP_BUILD_DIR}/include/libnet)

link_directories(${RSOCK_TOP_BUILD_DIR}/lib/${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}/)

message(STATUS "CMAKE_SYSROOT = ${CMAKE_SYSROOT}")
message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_AR = ${CMAKE_AR}")
message(STATUS "CMAKE_RANLIB = ${CMAKE_RANLIB}")
message(STATUS "CMAKE_ASM_COMPILER = ${CMAKE_ASM_COMPILER}")
message(STATUS "CMAKE_LINKER = ${CMAKE_LINKER}")
message(STATUS "CMAKE_NM = ${CMAKE_NM}")
message(STATUS "CMAKE_OBJCOPY = ${CMAKE_OBJCOPY}")
message(STATUS "CMAKE_OBJDUMP = ${CMAKE_OBJDUMP}")
message(STATUS "CMAKE_STRIP = ${CMAKE_STRIP}")

# Set or retrieve the cached flags.
# This is necessary in case the user sets/changes flags in subsequent
# configures. If we included the flags in here, they would get
# overwritten.
set(CMAKE_C_FLAGS ""
        CACHE STRING "Flags used by the compiler during all build types.")
set(CMAKE_CXX_FLAGS ""
        CACHE STRING "Flags used by the compiler during all build types.")
set(CMAKE_ASM_FLAGS ""
        CACHE STRING "Flags used by the compiler during all build types.")
set(CMAKE_C_FLAGS_DEBUG ""
        CACHE STRING "Flags used by the compiler during debug builds.")
set(CMAKE_CXX_FLAGS_DEBUG ""
        CACHE STRING "Flags used by the compiler during debug builds.")
set(CMAKE_ASM_FLAGS_DEBUG ""
        CACHE STRING "Flags used by the compiler during debug builds.")
set(CMAKE_C_FLAGS_RELEASE ""
        CACHE STRING "Flags used by the compiler during release builds.")
set(CMAKE_CXX_FLAGS_RELEASE ""
        CACHE STRING "Flags used by the compiler during release builds.")
set(CMAKE_ASM_FLAGS_RELEASE ""
        CACHE STRING "Flags used by the compiler during release builds.")
set(CMAKE_MODULE_LINKER_FLAGS ""
        CACHE STRING "Flags used by the linker during the creation of modules.")
set(CMAKE_SHARED_LINKER_FLAGS ""
        CACHE STRING "Flags used by the linker during the creation of dll's.")
set(CMAKE_EXE_LINKER_FLAGS ""
        CACHE STRING "Flags used by the linker.")

set(CMAKE_FIND_ROOT_PATH ${LINUX_TOOLCHAIN_ROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLYONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
#make VERVOSE=1 to output the log
