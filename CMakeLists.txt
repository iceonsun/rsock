cmake_minimum_required(VERSION 3.5)
project(rSock)

set(CMAKE_CXX_STANDARD 11)

include_directories(./include)

set(SOURCE_FILES
        src/rcommon.c src/ISockApp.cpp
        bean/RConfig.cpp bean/ConnInfo.cpp bean/TcpInfo.cpp bean/EncHead.cpp
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

##### release {
add_executable(server_rsock_${CMAKE_SYSTEM_NAME} ${SVR_SOURCE_FILES} server/main.cpp)
target_compile_options(server_rsock_${CMAKE_SYSTEM_NAME} PRIVATE -O2 -DRSOCK_IS_SERVER_ -DRSOCK_NNDEBUG)

add_executable(client_rsock_${CMAKE_SYSTEM_NAME} ${CLI_SOURCE_FILES} client/main.cpp)
target_compile_options(client_rsock_${CMAKE_SYSTEM_NAME} PRIVATE -O2 -DRSOCK_NNDEBUG)
##### }

add_definitions(-Wall -fvisibility=default)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wsign-compare")
set(libs uv net pcap pthread)
target_link_libraries(server_rsock_${CMAKE_SYSTEM_NAME} ${libs})
target_link_libraries(client_rsock_${CMAKE_SYSTEM_NAME} ${libs})

###### debug {
if (NOT RSOCK_RELEASE)
    MESSAGE(STATUS "Debug build")
    add_executable(test_server ${SVR_SOURCE_FILES} test/test_server.cpp)
    target_compile_options(test_server PRIVATE -pg -g -DRSOCK_IS_SERVER_)

    add_executable(test_client ${CLI_SOURCE_FILES} test/test_client.cpp)
    target_compile_options(test_client PRIVATE -pg -g)

    add_executable(echo_server src/rcommon.c test/udp_echo_server.cpp)
    add_executable(echo_client src/rcommon.c test/udp_echo_client.cpp)

    target_link_libraries(test_client ${libs})
    target_link_libraries(test_server ${libs})

    target_link_libraries(echo_client uv pthread)
    target_link_libraries(echo_server uv pthread)
else ()
    MESSAGE(STATUS "Release build")
endif ()
##### }
