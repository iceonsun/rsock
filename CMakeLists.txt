cmake_minimum_required(VERSION 3.5)
project(rSock)

set(CMAKE_CXX_STANDARD 11)

include_directories(./include)

set(SOURCE_FILES rcommon.c RConfig.cpp ISockApp.cpp conn/IConn.cpp PortMapper.cpp OHead.cpp
        util/enc.c util/rsutil.cpp util/FdUtil.cpp util/rhash.cpp util/RTimer.cpp util/TextUtils.h util/RPortList.cpp util/PortPair.cpp util/ProcUtil.cpp
        cap/cap_util.cpp cap/RCap.cpp
        thirdparty/md5.c thirdparty/json11.cpp
        tcp/TcpConnector.cpp tcp/SockMon.cpp tcp/LSockMon.cpp
        conn/INetConn.cpp conn/BtmUdpConn.cpp EncHead.cpp conn/FakeTcp.cpp conn/TcpInfo.cpp
        conn/INetGroup.cpp conn/IGroup.cpp conn/ConnInfo.cpp conn/FakeUdp.cpp
        conn/RConn.cpp conn/RawTcp.cpp conn/IAppGroup.cpp
        net/NetUtil.cpp net/ClientNetManager.cpp net/TcpAckPool.cpp net/NetListenPool.cpp net/INetManager.cpp net/ServerNetManager.cpp)

set(SVR_SOURCE_FILES ${SOURCE_FILES} server/SConn.cpp server/SSockApp.cpp server/ssock.cpp server/SNetGroup.cpp server/ServerGroup.cpp server/SubGroup.cpp)
set(CLI_SOURCE_FILES ${SOURCE_FILES} client/CConn.cpp client/csock.cpp client/CSockApp.cpp client/ClientGroup.cpp client/CNetGroup.cpp)

add_executable(ssock ${SVR_SOURCE_FILES} server/main.cpp)
set_target_properties(
        ssock
        PROPERTIES
        COMPILE_DEFINITIONS RSOCK_IS_SERVER_
        COMPILE_DEFINITIONS RSOCK_NNDEBUG   # don't output pcap in/out
)

add_executable(csock ${CLI_SOURCE_FILES} client/main.cpp)
set_target_properties(
        csock
        PROPERTIES
        COMPILE_DEFINITIONS RSOCK_NNDEBUG   # don't output pcap in/out
)

add_executable(test_server ${SVR_SOURCE_FILES} test/test_server.cpp)
set_target_properties(
        test_server
        PROPERTIES
        COMPILE_DEFINITIONS RSOCK_IS_SERVER_)

add_executable(test_client ${CLI_SOURCE_FILES} test/test_client.cpp)

add_executable(echo_server rcommon.c test/udp_echo_server.cpp)
add_executable(echo_client rcommon.c test/udp_echo_client.cpp)

add_definitions(-Wall)
#add_definitions(-DRLOG_FILE_PATH="/var/log/rsock/rsock.log")

# library
add_library(rawsock ${SOURCE_FILES})
set_target_properties(
        rawsock
        PROPERTIES
        COMPILE_DEFINITIONS RSOCK_NNDEBUG
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wsign-compare")

# change /usr/local/lib to the folder of your libnet.so/.a
find_library(NET_PATH net /usr/local/lib NO_DEFAULT_PATH)
link_libraries($NET_PATH)

find_library(UV_PATH uv /usr/local/lib NO_DEFAULT_UV_PATH)
link_libraries($UV_PATH)

find_library(CAP_PATH pcap /usr/lib NO_DEFAUL_CAP_PATH)
link_libraries($CAP_PATH)

if (APPLE)
    set(libs uv net pcap)
else ()
    set(libs uv net pcap pthread)
endif ()

target_link_libraries(rawsock ${libs})
target_link_libraries(ssock ${libs})
target_link_libraries(csock ${libs})

target_link_libraries(test_server ${libs})
target_link_libraries(test_client ${libs})
target_link_libraries(echo_client uv)
target_link_libraries(echo_server uv)
