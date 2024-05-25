cmake_minimum_required(VERSION 3.28)
project(kierki)

set(CMAKE_CXX_STANDARD 20)

set(CLIENT_DEPS strategy.cpp strategy.h player.h player.cpp player_auto.cpp player_auto.h client.cpp client.h
        player_console.cpp
        player_console.h
)
set(SERVER_DEPS io_worker.cpp io_worker.h io_worker_mgr.cpp io_worker_mgr.h io_worker_connect.cpp io_worker_client.h io_worker_handler.cpp io_worker_handler.h job_queue.cpp job_queue.h
        server.cpp
)
set(COMMON_DEPS utils.cpp utils.h common_types.h network_msg_parser.cpp network_msg_parser.h message.cpp message.h logger.cpp logger.h game_rules.cpp game_rules.h send_job.cpp send_job.h
        card.cpp
        constants.h)

FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})


set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -O0")

add_executable(kierki-client kierki-client.cpp ${CLIENT_DEPS} ${COMMON_DEPS})
add_executable(kierki-server kierki-server.cpp ${SERVER_DEPS} ${COMMON_DEPS})
TARGET_LINK_LIBRARIES(kierki-client ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(kierki-server ${Boost_LIBRARIES})
