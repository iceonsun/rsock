cmake_minimum_required(VERSION 3.9)
project(rSock)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES ktype.h rstype.h md5.c debug.c enc.c  rcommon.c  rsutil.c rscomm.h cap_headers.h om_util.h om_util.c
IUnit.cpp IUnit.cpp IConn.cpp IGroupConn.cpp SConn.cpp ServerGroupConn.cpp ClientGroupConn.cpp PortMapper.cpp
IRawConn.cpp CRawConn.cpp SRawConn.cpp)

add_executable(rSock ${SOURCE_FILES} main.cpp)


# change /usr/local/lib to the folder of your libnet.so/.a
find_library(NET_PATH net /usr/local/lib NO_DEFAULT_PATH)
link_libraries($NET_PATH)

find_library(UV_PATH uv /usr/local/lib NO_DEFAULT_UV_PATH)
link_libraries($UV_PATH)

target_link_libraries(rSock uv net)