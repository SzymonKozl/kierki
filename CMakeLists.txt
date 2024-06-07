cmake_minimum_required(VERSION 3.25)
project(kierki)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)

file(GLOB CLIENT_DEPS CONFIGURE_DEPENDS client/*.cpp client/*.h)
file(GLOB SERVER_DEPS CONFIGURE_DEPENDS server/*.cpp server/*.h)
file(GLOB COMMON_DEPS CONFIGURE_DEPENDS common/*.cpp common/*.h)

FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})


set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -O0")

add_executable(kierki-client client/kierki-client.cpp ${CLIENT_DEPS} ${COMMON_DEPS})
add_executable(kierki-server server/kierki-server.cpp ${SERVER_DEPS} ${COMMON_DEPS})
TARGET_LINK_LIBRARIES(kierki-client ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(kierki-server ${Boost_LIBRARIES})
