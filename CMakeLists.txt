cmake_minimum_required(VERSION 3.9)
project(rSock)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES ktype.h rcommon.c RConfig.cpp ISockApp.cpp IConn.cpp IGroupConn.cpp PortMapper.cpp IRawConn.cpp GroupConn.cpp OHead.cpp
        util/enc.c util/rsutil.c util/FdUtil.cpp util/rhash.cpp util/RTimer.cpp util/TextUtils.h
        cap/cap_util.cpp cap/RCap.cpp
        server/SConn.cpp server/ServerGroupConn.cpp server/SSockApp.cpp server/ssock.cpp server/SRawConn.cpp
        client/CRawConn.cpp client/RClient.cpp client/CConn.cpp client/csock.cpp client/ClientConn.cpp client/CSockApp.cpp client/csock.h
        thirdparty/md5.c thirdparty/debug.c thirdparty/json11.cpp
        )

add_executable(ssock ${SOURCE_FILES} server/main.cpp)
set_target_properties(
        ssock
        PROPERTIES
        COMPILE_DEFINITIONS RSOCK_IS_SERVER_
        COMPILE_DEFINITIONS RSOCK_NNDEBUG   # don't output pcap in/out
)

add_executable(csock ${SOURCE_FILES} client/main.cpp)
set_target_properties(
        csock
        PROPERTIES
        COMPILE_DEFINITIONS RSOCK_NNDEBUG   # don't output pcap in/out
)

add_executable(test_server ${SOURCE_FILES} test/test_server.cpp)
set_target_properties(
        test_server
        PROPERTIES
        COMPILE_DEFINITIONS RSOCK_IS_SERVER_)

add_executable(test_client ${SOURCE_FILES} test/test_client.cpp)
add_executable(test_conf ${SOURCE_FILES} test/test_conf.cpp)
add_executable(echo_server thirdparty/debug.c rcommon.c test/udp_echo_server.cpp)
add_executable(echo_client thirdparty/debug.c rcommon.c test/udp_echo_client.cpp)
add_executable(test_cap ${SOURCE_FILES} test/test_cap.cpp)

add_definitions(-Wall)

# library
add_library(rawsock ${SOURCE_FILES})

# change /usr/local/lib to the folder of your libnet.so/.a
find_library(NET_PATH net /usr/local/lib NO_DEFAULT_PATH)
link_libraries($NET_PATH)

find_library(UV_PATH uv /usr/local/lib NO_DEFAULT_UV_PATH)
link_libraries($UV_PATH)

find_library(CAP_PATH pcap /usr/lib NO_DEFAUL_CAP_PATH)
link_libraries($CAP_PATH)

set(libs uv net pcap)

#target_link_libraries(rsock uv net pcap)
target_link_libraries(rawsock ${libs})
target_link_libraries(ssock ${libs})
target_link_libraries(csock ${libs})

target_link_libraries(test_cap ${libs})
target_link_libraries(test_server ${libs})
target_link_libraries(test_client ${libs})
target_link_libraries(test_conf ${libs})
target_link_libraries(echo_client uv)
target_link_libraries(echo_server uv)
