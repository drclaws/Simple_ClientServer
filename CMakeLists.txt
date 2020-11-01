cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

# Path to common headers
set(INCLUDE_PATH simple_lib/include)

# Common static lib
project(simple_lib)
add_library(${PROJECT_NAME} STATIC simple_lib/src/common.cpp)

project(simple_server)
add_executable(${PROJECT_NAME} server/main.cpp)
    
target_sources(${PROJECT_NAME} PRIVATE server/ClientSession.cpp server/ClientSession.hpp)
target_sources(${PROJECT_NAME} PRIVATE server/Server.cpp server/Server.hpp)
target_sources(${PROJECT_NAME} PRIVATE server/TcpSession.cpp server/TcpSession.hpp)
target_sources(${PROJECT_NAME} PRIVATE server/UdpSession.cpp server/UdpSession.hpp)

target_sources(${PROJECT_NAME} PRIVATE server/message.cpp server/message.h)
target_sources(${PROJECT_NAME} PRIVATE server/session_result.h)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_PATH})
target_link_libraries(${PROJECT_NAME} simple_lib)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
include(CPack)
