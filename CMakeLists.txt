cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

# Path to common headers
set(INCLUDE_PATH simple_lib/include)

# Common static lib
add_library(simple_lib STATIC simple_lib/src/common.cpp simple_lib/src/Session.cpp)

# Server
project(simple_server)
add_executable(${PROJECT_NAME} server/main.cpp)
    
target_sources(${PROJECT_NAME} PRIVATE server/Server.cpp server/Server.hpp)
target_sources(${PROJECT_NAME} PRIVATE server/SessionServer.cpp server/SessionServer.hpp)
target_sources(${PROJECT_NAME} PRIVATE server/SessionTcp.cpp server/SessionTcp.hpp)
target_sources(${PROJECT_NAME} PRIVATE server/SessionUdp.cpp server/SessionUdp.hpp)

target_sources(${PROJECT_NAME} PRIVATE server/message.cpp server/message.h)
target_sources(${PROJECT_NAME} PRIVATE server/session_result.h)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_PATH})
target_link_libraries(${PROJECT_NAME} simple_lib)

# client
project(simple_client)
add_executable(${PROJECT_NAME} client/main.cpp)

target_sources(${PROJECT_NAME} PRIVATE client/MainConsole.cpp client/MainConsole.hpp)
target_sources(${PROJECT_NAME} PRIVATE client/SessionClient.cpp client/SessionClient.hpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_PATH})
target_link_libraries(${PROJECT_NAME} simple_lib)
