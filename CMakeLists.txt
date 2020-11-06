cmake_minimum_required(VERSION 3.10)
project(naruto)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)
include_directories(utils)
include_directories(unittest)
include_directories(connection)
include_directories(command)
include_directories(database)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)
link_directories(/usr/lib)
link_libraries(glog ev gflags protobuf gtest)

aux_source_directory(. NARUTO_SRC)
aux_source_directory(clients/c++ CLIENT_SRC)
aux_source_directory(utils UTILS_SRC)
aux_source_directory(unittest UT_SRC)
aux_source_directory(protocol PROTOCOL_SRC)
aux_source_directory(connection CONNECTION_SRC)
aux_source_directory(database DATABASE_SRC)
aux_source_directory(command COMMAND_SRC)
aux_source_directory(sink SINK_SRC)
list(REMOVE_ITEM NARUTO_SRC ./main.cpp)

#add_executable(client ./clients/example/c++/example.cpp
#        ${NARUTO_SRC}
#        ${UTILS_SRC}
#        ${PROTOCOL_SRC}
#        ${CONNECTION_SRC}
#        ${DATABASE_SRC}
#        ${COMMAND_SRC})

add_executable(naruto main.cpp
        ${NARUTO_SRC}
        ${UTILS_SRC}
        ${PROTOCOL_SRC}
        ${CONNECTION_SRC}
        ${DATABASE_SRC}
        ${COMMAND_SRC}
        ${SINK_SRC})

add_executable(ut ./unittest/ut_main.cpp
        ${NARUTO_SRC}
        ${UTILS_SRC}
        ${PROTOCOL_SRC}
        ${CONNECTION_SRC}
        ${DATABASE_SRC}
        ${COMMAND_SRC}
        ${SINK_SRC})