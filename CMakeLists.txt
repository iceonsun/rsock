cmake_minimum_required(VERSION 3.9)
project(rSock)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES ktype.h thirdparty/md5.c thirdparty/debug.c util/enc.c rcommon.c  rsutil.c rscomm.h
        IConn.cpp IGroupConn.cpp server/SConn.cpp server/ServerGroupConn.cpp client/ClientConn.cpp PortMapper.cpp
IRawConn.cpp server/SRawConn.cpp GroupConn.cpp OHead.cpp rhash.cpp util/RTimer.cpp
cap/cap_util.cpp cap/RCap.cpp
        client/CRawConn.cpp client/RClient.cpp client/CConn.cpp client/CConn.h)

add_executable(cSock ${SOURCE_FILES}  client/main.cpp)
add_executable(test_server ${SOURCE_FILES} test/test_server.cpp)
add_executable(test_client ${SOURCE_FILES} test/test_client.cpp)

add_executable(rSock ${SOURCE_FILES} main.cpp)
add_executable(test_cap ${SOURCE_FILES} test/test_cap.cpp)

# change /usr/local/lib to the folder of your libnet.so/.a
find_library(NET_PATH net /usr/local/lib NO_DEFAULT_PATH)
link_libraries($NET_PATH)

find_library(UV_PATH uv /usr/local/lib NO_DEFAULT_UV_PATH)
link_libraries($UV_PATH)

find_library(CAP_PATH pcap /usr/lib NO_DEFAUL_CAP_PATH)
link_libraries($CAP_PATH)

target_link_libraries(rSock uv net pcap)
target_link_libraries(test_cap uv net pcap)
target_link_libraries(cSock uv net pcap)
target_link_libraries(test_server uv net pcap)
target_link_libraries(test_client uv net pcap)