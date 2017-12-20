cmake_minimum_required(VERSION 3.5)
project(rSock)

set(CMAKE_CXX_STANDARD 11)

include_directories(./include)

set(SOURCE_FILES src/rcommon.c bean/RConfig.cpp src/ISockApp.cpp bean/ConnInfo.cpp bean/TcpInfo.cpp bean/EncHead.cpp
        callbacks/ITcpObserver.h callbacks/ITcpInformer.cpp callbacks/RConnReset.cpp callbacks/INetConnKeepAlive.h callbacks/NetConnKeepAlive.h
        callbacks/IReset.h callbacks/NetConnKeepAlive.cpp callbacks/ConnReset.cpp
        util/enc.c util/rsutil.cpp util/FdUtil.cpp util/rhash.cpp util/RTimer.cpp util/TextUtils.h util/RPortList.cpp
        util/PortPair.cpp util/ProcUtil.cpp util/Handler.cpp util/ShotHandler.cpp util/UvUtil.cpp
        cap/cap_util.cpp cap/RCap.cpp
        thirdparty/md5.c thirdparty/json11.cpp
        conn/INetConn.cpp conn/BtmUdpConn.cpp conn/FakeTcp.cpp conn/INetGroup.cpp conn/IGroup.cpp conn/FakeUdp.cpp
        conn/IConn.cpp conn/RConn.cpp conn/RawTcp.cpp conn/IAppGroup.cpp conn/IBtmConn.cpp
        net/NetUtil.cpp net/ClientNetManager.cpp net/TcpAckPool.cpp net/TcpListenPool.cpp net/INetManager.cpp
        net/ServerNetManager.cpp)

set(SVR_SOURCE_FILES ${SOURCE_FILES} server/SConn.cpp server/SSockApp.cpp server/ssock.cpp server/SNetGroup.cpp
        server/ServerGroup.cpp server/SubGroup.cpp)
set(CLI_SOURCE_FILES ${SOURCE_FILES} client/CConn.cpp client/csock.cpp client/CSockApp.cpp client/ClientGroup.cpp
        client/CNetGroup.cpp)

add_executable(ssock ${SVR_SOURCE_FILES} server/main.cpp)
set_target_properties(
        ssock
        PROPERTIES
        COMPILE_DEFINITIONS RSOCK_IS_SERVER_
        COMPILE_DEFINITIONS RSOCK_NNDEBUG
)

add_executable(csock ${CLI_SOURCE_FILES} client/main.cpp)
set_target_properties(
        csock
        PROPERTIES
        COMPILE_DEFINITIONS RSOCK_NNDEBUG
)

add_executable(test_server ${SVR_SOURCE_FILES} test/test_server.cpp)
set_target_properties(
        test_server
        PROPERTIES
        COMPILE_DEFINITIONS RSOCK_IS_SERVER_)

add_executable(test_client ${CLI_SOURCE_FILES} test/test_client.cpp)

add_executable(echo_server src/rcommon.c test/udp_echo_server.cpp)
add_executable(echo_client src/rcommon.c test/udp_echo_client.cpp)

add_definitions(-Wall -pg -g)
#add_definitions(-DRLOG_FILE_PATH="/var/log/rsock/rsock.log")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wsign-compare")

# set PLATFORM_LIB_PATH to the folder of your libnet.so/.a
if (APPLE)
    set(PLATFORM_LIB_PATH /usr/local/lib /usr/lib)
else ()  # linux
    set(PLATFORM_LIB_PATH /usr/lib/x86_64-linux-gnu/)
endif ()

find_library(NET_PATH net ${PLATFORM_LIB_PATH} NO_DEFAULT_PATH)
link_libraries($NET_PATH)

find_library(UV_PATH uv ${PLATFORM_LIB_PATH} NO_DEFAULT_UV_PATH)
link_libraries($UV_PATH)

find_library(CAP_PATH pcap ${PLATFORM_LIB_PATH} NO_DEFAUL_CAP_PATH)
link_libraries($CAP_PATH)

if (APPLE)
    set(libs uv net pcap)
else ()
    set(libs uv net pcap pthread)
endif ()

target_link_libraries(ssock ${libs})
target_link_libraries(csock ${libs})

target_link_libraries(test_server ${libs})
target_link_libraries(test_client ${libs})
target_link_libraries(echo_client uv)
target_link_libraries(echo_server uv)
