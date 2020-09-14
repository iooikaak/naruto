cmake_minimum_required(VERSION 3.15)
project(naruto)

set(CMAKE_CXX_STANDARD 11)

include_directories(.)
include_directories(utils)
include_directories(unittest)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)
link_directories(/usr/lib)
link_libraries(gtest glog ev gflags protobuf)

aux_source_directory(. NARUTO_SRC)
aux_source_directory(clients/c++ CLIENT_SRC)
aux_source_directory(net NET_SRC)
aux_source_directory(utils UTILS_SRC)
aux_source_directory(unittest UT_SRC)
aux_source_directory(protocol PROTOCOL_SRC)

#add_definitions(-w)

add_executable(client ./clients/example/c++/example.cpp ${CLIENT_SRC} ${NET_SRC} ${UTILS_SRC} ${PROTOCOL_SRC})

add_executable(naruto main.cpp ${NARUTO_SRC} ${CLIENT_SRC} ${NET_SRC} ${UTILS_SRC} ${PROTOCOL_SRC})

add_executable(ut ./unittest/ut_main.cpp ${CLIENT_SRC} ${NET_SRC} ${UTILS_SRC} ${PROTOCOL_SRC})