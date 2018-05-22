cmake_minimum_required(VERSION 3.5)
project(rSock)

set(CMAKE_CXX_STANDARD 11)

include_directories(./include)

include_directories(./src/os/include)

#include(xbuild/cmake/base.cmake)   # uncomment for vs local dev
set(SOURCE_FILES
        src/rcommon.c src/ISockApp.cpp
        bean/RConfig.cpp bean/ConnInfo.cpp bean/TcpInfo.cpp bean/EncHead.cpp
        callbacks/ITcpInformer.cpp callbacks/RConnReset.cpp callbacks/INetConnKeepAlive.h
        callbacks/IReset.h callbacks/NetConnKeepAlive.cpp callbacks/ConnReset.cpp
        util/enc.c util/rsutil.cpp util/rhash.cpp util/RTimer.cpp util/TextUtils.h util/RPortList.cpp
        util/PortPair.cpp util/Handler.cpp util/ShotHandler.cpp util/UvUtil.cpp
        cap/cap_util.cpp cap/RCap.cpp
        thirdparty/md5.c thirdparty/json11.cpp
        conn/INetConn.cpp conn/BtmUdpConn.cpp conn/FakeTcp.cpp conn/INetGroup.cpp conn/IGroup.cpp conn/FakeUdp.cpp
        conn/IConn.cpp conn/RConn.cpp conn/RawTcp.cpp conn/IAppGroup.cpp conn/IBtmConn.cpp conn/DefaultFakeConn.cpp
        conn/CConn.cpp
        net/NetUtil.cpp net/ClientNetManager.cpp net/TcpAckPool.cpp net/TcpListenPool.cpp net/INetManager.cpp
        net/ServerNetManager.cpp
        src/sync/SyncConnFactory.cpp src/sync/LoopSreamSyncConn.cpp src/sync/ISyncConn.cpp src/sync/UdpSyncConn.cpp
        src/sync/TcpStreamSyncConn.cpp src/sync/IPacketSyncConn.cpp
        )

# platform dependent dir		
SET(ROS_DIR src/os/unix)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	set(ROS_DIR src/os/win)
endif ()

# code {
# add all platform dependent code here
set(SOURCE_FILES ${SOURCE_FILES} ${ROS_DIR}/os_util.cpp ${ROS_DIR}/FdUtil.cpp ${ROS_DIR}/ProcUtil.cpp)

set(ROS_COMMON_DIR src/os/common)

# common code
set(SOURCE_FILES ${SOURCE_FILES} ${ROS_COMMON_DIR}/FdUtil_common.cpp)
if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	set(SOURCE_FILES ${SOURCE_FILES} src/os/unix/conn/UnixDgramSyncConn.cpp)
endif ()
MESSAGE(STATUS "source files: ${SOURCE_FILES}")
## } code

set(SVR_SOURCE_FILES ${SOURCE_FILES} server/SConn.cpp server/SSockApp.cpp server/ssock.cpp server/SNetGroup.cpp
        server/ServerGroup.cpp server/SubGroup.cpp)
set(CLI_SOURCE_FILES ${SOURCE_FILES} client/csock.cpp client/CSockApp.cpp client/ClientGroup.cpp
        client/CNetGroup.cpp)

##### release {
add_executable(server_rsock_${CMAKE_SYSTEM_NAME} ${SVR_SOURCE_FILES} server/main.cpp)
target_compile_options(server_rsock_${CMAKE_SYSTEM_NAME} PRIVATE -DRSOCK_IS_SERVER_ -DRSOCK_NNDEBUG)

add_executable(client_rsock_${CMAKE_SYSTEM_NAME} ${CLI_SOURCE_FILES} client/main.cpp)
target_compile_options(client_rsock_${CMAKE_SYSTEM_NAME} PRIVATE  -DRSOCK_NNDEBUG)
##### }


if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")	
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wsign-compare")
	add_definitions(-Wall -fvisibility=default)
endif ()

set(libs uv net )
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	set(OS_LIBS ws2_32 IPHLPAPI Psapi userenv advapi32 shell32)
	set(libs ${libs} wpcap packet ${OS_LIBS})
else ()
	set(OS_LIBS pthread)
	set(libs ${libs} pcap ${OS_LIBS})
endif ()
MESSAGE(STATUS "libs for rsock binary: ${libs}")
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

    target_link_libraries(test_client ${libs} ${OS_LIBS})
    target_link_libraries(test_server ${libs} ${OS_LIBS})

    target_link_libraries(echo_client uv ${OS_LIBS}) 
    target_link_libraries(echo_server uv ${OS_LIBS})
else ()
    MESSAGE(STATUS "Release build")
endif ()
##### }
